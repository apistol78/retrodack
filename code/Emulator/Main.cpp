#include <cstring>

// FatFS
#include <ff.h>
#include <diskio.h>

// Traktor
#include <Core/Io/MemoryStream.h>
#include <Core/Log/Log.h>
#include <Core/Misc/CommandLine.h>

// Klara-RV
#include <cpu/Bus.h>
#include <cpu/CPU.h>
#include <devices/Audio.h>
#include <devices/Memory.h>
#include <devices/PLIC.h>
#include <devices/SD.h>
#include <devices/Timer.h>
#include <devices/UART.h>
#include <devices/Video.h>

//
#include "Emulator/LoadELF.h"

using namespace traktor;



constexpr uint32_t fs_image_size = 4 * 1024 * 1024;
uint8_t fs_image[fs_image_size];


DSTATUS disk_initialize (BYTE pdrv)
{
	//log::info << L"disk_initialize" << Endl;
	return 0;
}

DSTATUS disk_status (BYTE pdrv)
{
	//log::info << L"disk_status" << Endl;
	return 0;
}

DRESULT disk_read (BYTE pdrv, BYTE* buff, LBA_t sector, UINT count)
{
	//log::info << L"disk_read, sector " << sector << L", count " << count << Endl;
	const uint32_t offset = sector * 512;
	const uint32_t size = count * 512;
	std::memcpy(buff, &fs_image[offset], size);
	return RES_OK;
}

DRESULT disk_write (BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count)
{
	//log::info << L"disk_write, sector " << sector << L", count " << count << Endl;
	const uint32_t offset = sector * 512;
	const uint32_t size = count * 512;
	std::memcpy(&fs_image[offset], buff, size);
	return RES_OK;
}

DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void* buff)
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

	std::memset(fs_image, 0, fs_image_size);

	res = f_mkfs("", NULL, work, sizeof(work));
	if (res) return false;

	res = f_mount(&fs, "", 0);
	if (res) return false;

	return true;
}


int main(int argc, const char** argv)
{
	const CommandLine cmdLine(argc, argv);

	// Create file system from files.
	if (!createFsImage())
	{
		log::error << L"Unable to create in-memory file system!" << Endl;
		return 1;
	}

	// Create emulation devices.
	Memory rom(0x00100000);
    Memory ram(0x00001000);
	Memory sdram(0x00010000);
	Video video(720, 720);
	UART uart;
	// Unknown i2c(L"I2C", true);
	SD sd(new MemoryStream(fs_image, fs_image_size));
	::Timer tmr;
	PLIC plic;
	// Audio audio;

	Bus bus;
	bus.map(0x00000000, 0x00000000 + 0x00020000, false, false, &rom);
    bus.map(0x10000000, 0x10000000 + 0x00001000, false, false, &ram);
	bus.map(0x20000000, 0x20000000 + 0x01000000, true, false, &sdram);
	bus.map(0x51000000, 0x51000100, false, false, &uart);
	// bus.map(0x53000000, 0x53000100, false, false, &i2c);
	bus.map(0x54000000, 0x54000100, false, true, &sd);
	bus.map(0x55000000, 0x55000100, false, true, &tmr);
	// bus.map(0x56000000, 0x56000100, false, true, &audio);
	bus.map(0x58000000, 0x58004000, false, false, &plic);
	bus.map(0x5a000000, 0x5b000000, false, false, &video);

    CPU cpu(&bus, nullptr, false);

    cpu.setSP(0x10000000 + 0x00001000 - 4);

	if (cmdLine.hasOption(L'e', L"elf"))
	{
		const std::wstring fileName = cmdLine.getOption(L'e', L"elf").getString();
		if (!loadELF(fileName, cpu, bus))
			return 1;
	}
    
    rom.setReadOnly(true);
    
    for (;;)
    {
        cpu.tick();
    }

    return 0;
}
