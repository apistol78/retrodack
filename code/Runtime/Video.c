/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include <stdlib.h>
#include <string.h>

#include <HAL/DMA.h>

#include "Runtime/Kernel.h"
#include "Runtime/Video.h"

int32_t rt_video_init()
{
	return hal_video_init();
}

int32_t rt_video_set_mode(int32_t mode)
{
	return hal_video_set_mode(mode);
}

void* rt_video_create_target()
{
	const uint32_t pixels = hal_video_get_resolution_width() * hal_video_get_resolution_height();
	
	void* target = malloc(pixels);
	if (!target)
		return 0;

	memset(target, 0, pixels);
	return target;
}

void rt_video_destroy_target(void* target)
{
	free(target);
}

int32_t rt_video_get_resolution_width()
{
	return hal_video_get_resolution_width();
}

int32_t rt_video_get_resolution_height()
{
	return hal_video_get_resolution_height();
}

void rt_video_set_palette(uint8_t index, uint32_t color)
{
	return hal_video_set_palette(index, color);
}

void* rt_video_get_primary_target()
{
	return hal_video_get_primary_target();
}

void* rt_video_get_secondary_target()
{
	return hal_video_get_secondary_target();
}

void rt_video_clear(uint8_t idx)
{
	const uint32_t pixels = hal_video_get_resolution_width() * hal_video_get_resolution_height();
	uint8_t* target = (uint8_t*)hal_video_get_secondary_target();
	
	const uint32_t value = (idx << 24) | (idx << 16) | (idx << 8) | idx;
	hal_dma_write(target, pixels >> 2, value);
}

void rt_video_blit(const void* source)
{
	const uint32_t pixels = hal_video_get_resolution_width() * hal_video_get_resolution_height();
	uint8_t* target = (uint8_t*)hal_video_get_secondary_target();
	
	__asm__ volatile ( "fence" );
	hal_dma_copy(target, source, pixels >> 2);
}

void rt_video_wait()
{
	while (hal_dma_is_busy())
		kernel_yield();
}

void rt_video_present()
{
	hal_video_present();
}
