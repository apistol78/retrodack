/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ff.h>
#include <diskio.h>

#include "Runtime/File.h"
#include "Runtime/Kernel.h"
#include "Runtime/Internal/Disk.h"

// FatFs hooks

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
	for (UINT i = 0; i < count; ++i)
	{
		if (rt_disk_read_block512(sector + i, buff, 512) != 512)
			return RES_ERROR;
		buff += 512;
	}
	return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count)
{
	for (UINT i = 0; i < count; ++i)
	{
		if (rt_disk_write_block512(sector + i, buff, 512) != 512)
			return RES_ERROR;
		buff += 512;
	}
	return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff)
{
	if (cmd == GET_SECTOR_SIZE)
		*(WORD*)buff = 512;
	else if (cmd == GET_BLOCK_SIZE)
		*(DWORD*)buff = 512;
	return RES_OK;
}

// HAL

kernel_cs_t lock = { 0 };
FATFS fsInternal;
FIL fps[32];
uint32_t fpa = 0;

static FIL* file_alloc()
{
	for (int32_t i = 0; i < 32; ++i)
	{
		if ((fpa & (1 << i)) == 0)
		{
			fpa |= (1 << i);
			return &fps[i];
		}
	}
	return 0;
}

static void file_free(FIL* fp)
{
	const int32_t index = fp - fps;
	fpa &= ~(1 << index);
}

static int32_t file_index(FIL* fp)
{
	return (int32_t)(fp - fps) + 1;
}

static int32_t file_is_open(int32_t index)
{
	if (index >= 1 && index <= 32)
		return (fpa & (1 << (index - 1))) != 0;
	else
		return 0;
}

static FIL* file_from_index(int32_t index)
{
	if (file_is_open(index))
		return &fps[index - 1];
	else
		return 0;
}

// public

int32_t file_init()
{
	memset(&fsInternal, 0, sizeof(fsInternal));
	memset(fps, 0, sizeof(fps));

	fpa = 0;

	rt_disk_init();

	if (f_mount(&fsInternal, "", 1) != FR_OK)
		return 1;

	return 0;
}

void file_shutdown()
{
	f_unmount("");
	rt_disk_shutdown();
}

int32_t file_open(const char* name, int32_t mode)
{
	rt_kernel_cs_lock(&lock);

	FIL* fp = file_alloc();
	if (!fp)
	{
		rt_kernel_cs_unlock(&lock);
		return 0;
	}

	FRESULT r = FR_INVALID_PARAMETER;
	if (mode == FILE_MODE_READ)
	{
		if ((r = f_open(fp, name, FA_READ)) == FR_OK)
		{
			const int32_t index = file_index(fp);
			rt_kernel_cs_unlock(&lock);
			return index;
		}
	}
	else if (mode == FILE_MODE_WRITE)
	{
		if ((r = f_open(fp, name, FA_CREATE_ALWAYS | FA_WRITE)) == FR_OK)
		{
			const int32_t index = file_index(fp);
			rt_kernel_cs_unlock(&lock);
			return index;
		}
	}

	file_free(fp);
	rt_kernel_cs_unlock(&lock);
	return 0;
}

void file_close(int32_t fd)
{
	rt_kernel_cs_lock(&lock);

	FIL* fp = file_from_index(fd);
	if (fp)
	{
		f_close(fp);
		file_free(fp);
	}

	rt_kernel_cs_unlock(&lock);
}

int32_t file_size(int32_t fd)
{
	int32_t fs = -1;

	rt_kernel_cs_lock(&lock);
	FIL* fp = file_from_index(fd);
	if (fp)
		fs = f_size(fp);
	rt_kernel_cs_unlock(&lock);

	return fs;
}

int32_t file_seek(int32_t fd, int32_t offset, int32_t from)
{
	FRESULT result = FR_INVALID_PARAMETER;

	rt_kernel_cs_lock(&lock);

	FIL* fp = file_from_index(fd);
	if (!fp)
	{
		rt_kernel_cs_unlock(&lock);
		return -1;
	}

	if (from == FILE_SEEK_SET)
		result = f_lseek(fp, offset);
	else if (from == FILE_SEEK_CUR)
	{
		int32_t pos = f_tell(fp) + offset;
		if (pos < 0)
			pos = 0;
		else if (pos >= f_size(fp))
			pos = f_size(fp) - 1;
		result = f_lseek(fp, pos);
	}
	else if (from == FILE_SEEK_END)
	{
		int32_t pos = f_size(fp) + offset;
		if (pos < 0)
			pos = 0;
		else if (pos >= f_size(fp))
			pos = f_size(fp) - 1;
		result = f_lseek(fp, pos);
	}

	int32_t ft = -1;

	if (result == FR_OK)
		ft = f_tell(fp);

	rt_kernel_cs_unlock(&lock);
	return ft;
}

int32_t file_write(int32_t fd, const void* ptr, int32_t len)
{
	FRESULT result;
	UINT bw = 0;

	rt_kernel_cs_lock(&lock);

	FIL* fp = file_from_index(fd);
	if (!fp)
	{
		rt_kernel_cs_unlock(&lock);
		return -1;
	}

	if ((result = f_write(fp, ptr, len, &bw)) == FR_OK)
	{
		rt_kernel_cs_unlock(&lock);
		return (int32_t)bw;
	}
	else
	{
		rt_kernel_cs_unlock(&lock);
		return -2;
	}
}

int32_t file_read(int32_t fd, void* ptr, int32_t len)
{
	rt_kernel_cs_lock(&lock);

	FIL* fp = file_from_index(fd);
	if (!fp)
	{
		rt_kernel_cs_unlock(&lock);
		return -1;
	}

	UINT br = 0;
	FRESULT result = f_read(fp, ptr, len, &br);
	if (result == FR_OK)
	{
		rt_kernel_cs_unlock(&lock);
		return (int32_t)br;
	}
	else
	{
		rt_kernel_cs_unlock(&lock);
		return -2;
	}
}

int32_t file_enumerate(const char* path, void* user, fn_enum_t fnen)
{
	FILINFO fno;
	DIR dp;

	if (f_opendir(&dp, path) != FR_OK)
		return 1;

	while (f_readdir(&dp, &fno) == FR_OK && fno.fname[0] != 0)
		fnen(user, fno.fname, fno.fsize, (fno.fattrib & AM_DIR) != 0);

	f_closedir(&dp);
	return 0;
}

int32_t file_remove(const char* filename)
{
	return f_unlink(filename) == FR_OK ? 0 : 1;
}

