#include <cstring>

// FatFS
#include <ff.h>
#include <diskio.h>

// Traktor
#include <Core/Io/FileSystem.h>
#include <Core/Io/IStream.h>
#include <Core/Log/Log.h>
#include <Core/Misc/TString.h>

//
#include "Emulator/FileSystemImage.h"

using namespace traktor;

namespace
{

uint8_t* s_imageData = nullptr;
uint32_t s_imageSize = 0;

}

DSTATUS disk_initialize(BYTE pdrv)
{
	return 0;
}

DSTATUS disk_status(BYTE pdrv)
{
	return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count)
{
	const uint32_t offset = sector * 512;
	const uint32_t size = count * 512;
	std::memcpy(buff, &s_imageData[offset], size);
	return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count)
{
	const uint32_t offset = sector * 512;
	const uint32_t size = count * 512;
	std::memcpy(&s_imageData[offset], buff, size);
	return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff)
{
	if (cmd == GET_SECTOR_SIZE)
		*(WORD*)buff = 512;
	else if (cmd == GET_BLOCK_SIZE)
		*(DWORD*)buff = 512;
	else if (cmd == GET_SECTOR_COUNT)
		*(DWORD*)buff = s_imageSize / 512;
	return RES_OK;
}

T_IMPLEMENT_RTTI_CLASS(L"FileSystemImage", FileSystemImage, Object)

Ref< FileSystemImage > FileSystemImage::createFromDirectory(const Path& path)
{
	BYTE work[FF_MAX_SS];
	FRESULT res;
	FATFS fs;
	FIL f;
	UINT bw;

	uint64_t imageSize = 0;
	for (auto ff : FileSystem::getInstance().find(path.getPathNameOS() + L"/*.*"))
	{
		if (ff->isDirectory())
			continue;

		imageSize += ff->getSize();
	}
	log::info << L"FS image size " << imageSize << L" byte(s)." << Endl;

	Ref< FileSystemImage > image = new FileSystemImage();
	image->m_data.resize(imageSize + 1 * 1024 * 1024, 0);

	s_imageData = image->m_data.ptr();
	s_imageSize = (uint32_t)image->m_data.size();

	res = f_mkfs("", NULL, work, sizeof(work));
	if (res)
	{
		log::error << L"f_mkfs failed, FRESULT = " << (int32_t)res << Endl;
		return nullptr;
	}

	res = f_mount(&fs, "", 0);
	if (res)
	{
		log::error << L"f_mount failed, FRESULT = " << (int32_t)res << Endl;
		return nullptr;
	}

	for (auto ff : FileSystem::getInstance().find(path.getPathNameOS() + L"/*.*"))
	{
		if (ff->isDirectory())
			continue;

		Ref< traktor::IStream > sf = FileSystem::getInstance().open(ff->getPath(), File::FmRead);
		if (sf)
		{
			char fn[512];
			strcpy(fn, wstombs(ff->getPath().getFileName()).c_str());

			res = f_open(&f, fn, FA_CREATE_ALWAYS | FA_WRITE);
			if (res)
				return nullptr;

			int64_t size = 0;
			for (;;)
			{
				const int64_t nrd = sf->read(work, sizeof(work));
				if (nrd <= 0)
					break;

				res = f_write(&f, work, (UINT)nrd, &bw);
				if (res)
					return nullptr;

				size += nrd;
			}

			f_close(&f);
		}
	}

	f_unmount("");
	return image;
}
