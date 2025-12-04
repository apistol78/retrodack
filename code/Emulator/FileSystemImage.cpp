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

void collectAllFiles(const Path& path, RefArray< File >& outFiles)
{
	for (auto ff : FileSystem::getInstance().find(path.getPathNameOS() + L"/*.*"))
	{
		if (ff->getPath().getFileName() == L"." || ff->getPath().getFileName() == L"..")
			continue;

		outFiles.push_back(ff);

		if (ff->isDirectory())
			collectAllFiles(ff->getPath(), outFiles);
	}
}

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

	// Recursively collect all files.
	RefArray< File > files;
	collectAllFiles(path, files);

	// Estimate image size.
	uint64_t imageSize = 0;
	for (auto ff : files)
	{
		if (ff->isDirectory())
			continue;

		imageSize += ff->getSize();
	}
	log::info << L"[FS] Image size " << imageSize << L" byte(s)." << Endl;

	// Allocate image.
	Ref< FileSystemImage > image = new FileSystemImage();
	image->m_data.resize(imageSize + 1 * 1024 * 1024, 0);

	s_imageData = image->m_data.ptr();
	s_imageSize = (uint32_t)image->m_data.size();

	res = f_mkfs("", NULL, work, sizeof(work));
	if (res)
	{
		log::error << L"[FS] f_mkfs failed, FRESULT = " << (int32_t)res << Endl;
		return nullptr;
	}

	res = f_mount(&fs, "", 0);
	if (res)
	{
		log::error << L"[FS] f_mount failed, FRESULT = " << (int32_t)res << Endl;
		return nullptr;
	}

	// Create all directories first.
	for (auto ff : files)
	{
		if (!ff->isDirectory())
			continue;

		const Path base = FileSystem::getInstance().getAbsolutePath(L"fs");
		const Path p = FileSystem::getInstance().getAbsolutePath(ff->getPath());

		Path rp;
		FileSystem::getInstance().getRelativePath(
			p,
			base,
			rp
		);

		log::info << L"[FS] Creating directory \"" << rp.getPathName() << L"\"" << Endl;

		char fn[512];
		strcpy(fn, wstombs(rp.getPathName()).c_str());

		f_mkdir(fn);
	}

	// Copy all files.
	for (auto ff : files)
	{
		if (ff->isDirectory())
			continue;

		Ref< traktor::IStream > sf = FileSystem::getInstance().open(ff->getPath(), File::FmRead);
		if (sf)
		{
			const Path base = FileSystem::getInstance().getAbsolutePath(L"fs");
			const Path p = FileSystem::getInstance().getAbsolutePath(ff->getPath());

			Path rp;
			FileSystem::getInstance().getRelativePath(
				p,
				base,
				rp
			);

			log::info << L"[FS] Copying file \"" << rp.getPathName() << L"\"" << Endl;

			char fn[512];
			strcpy(fn, wstombs(rp.getPathName()).c_str());

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
