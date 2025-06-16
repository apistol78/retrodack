#if defined(__LINUX__) || defined(__RPI__) || defined(__APPLE__)
#	include <sys/types.h>
#	include <sys/stat.h>
#	include <signal.h>
#	include <stdio.h>
#	include <stdlib.h>
#	include <fcntl.h>
#	include <errno.h>
#	include <unistd.h>
#	include <syslog.h>
#	include <string.h>
#endif

#include <cmath>
#include <cstring>

// FatFS
#include <ff.h>
#include <diskio.h>

// Traktor
#include <Core/Io/FileOutputStream.h>
#include <Core/Io/FileSystem.h>
#include <Core/Io/MemoryStream.h>
#include <Core/Io/Utf8Encoding.h>
#include <Core/Log/Log.h>
#include <Core/Misc/CommandLine.h>
#include <Core/Misc/TString.h>
#include <Core/Timer/Timer.h>
#include <Drawing/Image.h>
#include <Ui/Application.h>
#include <Ui/Bitmap.h>
#include <Ui/Image.h>
#include <Ui/Form.h>
#include <Ui/FloodLayout.h>
#if defined(_WIN32)
#	include <Ui/Win32/WidgetFactoryWin32.h>
#elif defined(__APPLE__)
#	include <Ui/Cocoa/WidgetFactoryCocoa.h>
#elif defined(__LINUX__) || defined(__RPI__)
#	include <Ui/X11/WidgetFactoryX11.h>
#endif

// Klara-RV
#include <Emulator/CPU/Bus.h>
#include <Emulator/CPU/CPU.h>
#include <Emulator/Devices/Audio.h>
#include <Emulator/Devices/I2C.h>
#include <Emulator/Devices/Memory.h>
#include <Emulator/Devices/PLIC.h>
#include <Emulator/Devices/SD.h>
#include <Emulator/Devices/Timer.h>
#include <Emulator/Devices/UART.h>
#include <Emulator/Devices/Video.h>
#include <Emulator/Devices/Unknown.h>

//
#include "Emulator/LoadELF.h"
#include "Emulator/LoadHEX.h"

using namespace traktor;

constexpr uint32_t fs_image_size = 32 * 1024 * 1024;
uint8_t fs_image[fs_image_size];

DSTATUS disk_initialize(BYTE pdrv)
{
	//log::info << L"disk_initialize" << Endl;
	return 0;
}

DSTATUS disk_status(BYTE pdrv)
{
	//log::info << L"disk_status" << Endl;
	return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count)
{
	//log::info << L"disk_read, sector " << sector << L", count " << count << Endl;
	const uint32_t offset = sector * 512;
	const uint32_t size = count * 512;
	std::memcpy(buff, &fs_image[offset], size);
	return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count)
{
	//log::info << L"disk_write, sector " << sector << L", count " << count << Endl;
	const uint32_t offset = sector * 512;
	const uint32_t size = count * 512;
	std::memcpy(&fs_image[offset], buff, size);
	return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff)
{
	//log::info << L"disk_ioctl" << Endl;
	if (cmd == GET_SECTOR_SIZE)
		*(WORD*)buff = 512;
	else if (cmd == GET_BLOCK_SIZE)
		*(DWORD*)buff = 512;
	else if (cmd == GET_SECTOR_COUNT)
		*(DWORD*)buff = fs_image_size / 512;
	return RES_OK;
}

bool createFsImage()
{
	BYTE work[FF_MAX_SS];
	FRESULT res;
	FATFS fs;
	FIL f;
	UINT bw;

	std::memset(fs_image, 0, fs_image_size);

	res = f_mkfs("", NULL, work, sizeof(work));
	if (res) return false;

	res = f_mount(&fs, "", 0);
	if (res) return false;

	for (auto ff : FileSystem::getInstance().find(L"fs/*.*"))
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
			{
				log::error << L"Unable to create FS image file \"" << ff->getPath().getFileName() << L"\"." << Endl;
				return false;
			}

			int64_t size = 0;
			for (;;)
			{
				const int64_t nrd = sf->read(work, sizeof(work));
				if (nrd <= 0)
					break;

				res = f_write(&f, work, (UINT)nrd, &bw);
				if (res)
					return false;

				size += nrd;
			}

			f_close(&f);

			log::info << L"Added \"" << ff->getPath().getFileName() << L"\" (" << size << L" bytes) to FS image." << Endl;
		}
	}

	f_unmount("");
	return true;
}

bool g_going = true;

#if defined(__LINUX__) || defined(__RPI__) || defined(__APPLE__)
void abortHandler(int s)
{
	g_going = false;
}
#endif

int main(int argc, const char** argv)
{
	const CommandLine cmdLine(argc, argv);

#if defined(__LINUX__) || defined(__RPI__) || defined(__APPLE__)
	{
		struct sigaction sa = { SIG_IGN };
		sigaction(SIGPIPE, &sa, nullptr);
	}
	{
		struct sigaction sa;
		sa.sa_handler = abortHandler;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sigaction(SIGINT, &sa, nullptr);
	}
#endif

#if defined(_WIN32)
	ui::Application::getInstance()->initialize(
		new ui::WidgetFactoryWin32(),
		nullptr
	);
#elif defined(__APPLE__)
	ui::Application::getInstance()->initialize(
		new ui::WidgetFactoryCocoa(),
		nullptr
	);
#elif defined(__LINUX__) || defined(__RPI__)
	ui::Application::getInstance()->initialize(
		new ui::WidgetFactoryX11(),
		nullptr
	);
#endif

	// Create file system from files.
	if (!createFsImage())
	{
		log::error << L"Unable to create in-memory file system!" << Endl;
		return 1;
	}

	// Create emulation devices.
	Memory rom(0x00100000);
 	Memory sdram(0x02000000);
	Video video(720, 720);
	UART uart;
	I2C i2c;
	SD sd(fs_image, fs_image_size);
	::Timer tmr;
	PLIC plic;
	Audio audio;

	Bus bus;
	bus.map(0x00000000, 0x00000000 + rom.getCapacity(), false, false, &rom);
 	bus.map(0x20000000, 0x20000000 + sdram.getCapacity(), true, false, &sdram);
	bus.map(0x51000000, 0x51000100, false, false, &uart);
	bus.map(0x53000000, 0x53000100, false, false, &i2c);
	bus.map(0x54000000, 0x54000100, false, true, &sd);
	bus.map(0x55000000, 0x55000100, false, true, &tmr);
	bus.map(0x56000000, 0x56000100, false, true, &audio);
	bus.map(0x58000000, 0x58004000, false, false, &plic);
	bus.map(0x5a000000, 0x5b000000, false, false, &video);

	Ref< OutputStream > os = nullptr;	
	if (cmdLine.hasOption(L't', L"trace"))
	{
		Ref< traktor::IStream > f = FileSystem::getInstance().open(L"CPU.trace", File::FmWrite);
		if (f)
			os = new FileOutputStream(f, new Utf8Encoding());
	}

	CPU cpu(&bus, os, false);
	cpu.setSP(0x22000000 - 4);

	tmr.setCallback([&](){ cpu.interrupt(TIMER); });

	if (cmdLine.hasOption(L'e', L"elf"))
	{
		const std::wstring fileName = cmdLine.getOption(L'e', L"elf").getString();
		if (!loadELF(fileName, cpu, bus))
			return 1;
		if (bus.error())
		{
			log::error << L"BUS error after loading ELF." << Endl;
			return 1;
		}
	}
	
	if (cmdLine.hasOption(L'h', L"hex"))
	{
		sdram.setReadOnly(true);

		const std::wstring fileName = cmdLine.getOption(L'h', L"hex").getString();
		if (!loadHEX(fileName, cpu, bus))
			return 1;
		if (bus.error())
		{
			log::error << L"BUS error after loading HEX." << Endl;
			return 1;
		}

		sdram.setReadOnly(false);
	}

	rom.setReadOnly(true);
	

	// Create user interface.
	Ref< ui::Form > form = new ui::Form();
	form->create(L"RetroDACK", 720_ut, 720_ut, ui::Form::WsDefault, new ui::FloodLayout());

	Ref< ui::Bitmap > uiImage = new ui::Bitmap(720, 720);
	
	Ref< ui::Image > image = new ui::Image();
	image->create(form, uiImage, ui::Image::WsScale | ui::Image::WsNearestFilter);

	form->update();
	form->show();


	traktor::Timer timer;
	while (g_going)
	{
		for (int32_t i = 0; i < 100 && g_going; ++i)
		{
			if (!cpu.tick(10000) || bus.error())
			{
				g_going = false;
				break;
			}
		}

		if (!ui::Application::getInstance()->process())
			break;

		if (timer.getElapsedTime() > 1.0f / 60.0f)
		{
			drawing::Image* videoImage = video.getImage();
			if (videoImage)
			{
				if (uiImage)
				{
					ui::Size sz = uiImage->getSize(form);
					if (sz.cx != videoImage->getWidth() || sz.cy != videoImage->getHeight())
					{
						uiImage->destroy();
						uiImage->create(videoImage);
					}
					else
						uiImage->copyImage(videoImage);
				}
				image->setImage(uiImage);
			}
			timer.reset();
			// plic.raise(0);
		}			
	}

	if (form)
	{
		form->destroy();
		form = nullptr;
	}

	return 0;
}
