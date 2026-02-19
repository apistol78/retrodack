/*
 RetroDÄCK
 Copyright (c) 2025-2026 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <HAL/SD.h>

#include "Runtime/Kernel.h"
#include "Runtime/Internal/Disk.h"

#define CACHE_SIZE 128

uint32_t s_cacheSectors[CACHE_SIZE];
uint8_t* s_cacheSectorBufs[CACHE_SIZE];

void rt_disk_init()
{
	for (uint32_t i = 0; i < CACHE_SIZE; ++i)
	{
		s_cacheSectors[i] = ~0;
		s_cacheSectorBufs[i] = 0;
	}
}

void rt_disk_shutdown()
{
	for (uint32_t i = 0; i < CACHE_SIZE; ++i)
	{
		s_cacheSectors[i] = ~0;
		if (s_cacheSectorBufs[i])
		{
			free(s_cacheSectorBufs[i]);
			s_cacheSectorBufs[i] = 0;
		}
	}
}

int32_t rt_disk_read_block512(uint32_t block, uint8_t* buffer, uint32_t bufferLen)
{
	const uint32_t s = block;
	const uint32_t c = s & (CACHE_SIZE - 1);

	rt_kernel_enter_critical();

	// Check if sector is cached.
	if (s_cacheSectors[c] == s)
	{
		memcpy(buffer, s_cacheSectorBufs[c], 512);
		rt_kernel_leave_critical();
		return 512;
	}

	// Not cached, load from SD.
	if (s_cacheSectorBufs[c] == 0)
	{
		// Allocate cached shadow; assume malloc return 4-byte
		// aligned pointer for the HAL.
		s_cacheSectorBufs[c] = malloc(512);
	}

	if (hal_sd_read_block512(s, s_cacheSectorBufs[c], 512) != 512)
	{
		rt_kernel_leave_critical();
		return 0;
	}

	memcpy(buffer, s_cacheSectorBufs[c], 512);
	s_cacheSectors[c] = s;

	rt_kernel_leave_critical();
	return 512;
}

int32_t rt_disk_write_block512(uint32_t block, const uint8_t* buffer, uint32_t bufferLen)
{
	const uint32_t s = block;
	const uint32_t c = s & (CACHE_SIZE - 1);

	rt_kernel_enter_critical();

	if (s_cacheSectors[c] == s)
		s_cacheSectors[c] = ~0;

	if (hal_sd_write_block512(s, buffer, 512) != 512)
	{
		rt_kernel_leave_critical();
		return 0;
	}

	rt_kernel_leave_critical();
	return 512;
}
