/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <HAL/DMA.h>
#include <HAL/Interrupt.h>

#include "Runtime/Kernel.h"
#include "Runtime/Video.h"

#define DMA_CHANNEL DMA_1_BASE

static uint32_t s_dma_tag = 0;
static kernel_sig_t s_vblank_signal;

static void video_interrupt(uint32_t source)
{
	rt_kernel_sig_raise(&s_vblank_signal);
}

void rt_video_init()
{
	hal_video_init();

	// Register vblank interrupt.
	rt_kernel_sig_init(&s_vblank_signal);
	hal_interrupt_set_handler(IRQ_SOURCE_PLIC_2, video_interrupt);
}

int32_t rt_video_set_mode(int32_t mode)
{
	return hal_video_set_mode(mode);
}

void* rt_video_create_target()
{
	const uint32_t size = hal_video_get_page_size();
	
	void* target = malloc(size);
	if (!target)
		return 0;

	memset(target, 0, size);
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
	const uint32_t size = hal_video_get_page_size();
	uint8_t* target = (uint8_t*)hal_video_get_secondary_target();
	
	const uint32_t value = (idx << 24) | (idx << 16) | (idx << 8) | idx;
	s_dma_tag = hal_dma_write(DMA_CHANNEL, target, size >> 2, value);
}

void rt_video_blit(const void* source)
{
	const uint32_t size = hal_video_get_page_size();
	uint8_t* target = (uint8_t*)hal_video_get_secondary_target();
	
	__asm__ volatile ( "fence" );
	s_dma_tag = hal_dma_copy(DMA_CHANNEL, target, source, size >> 2);
}

void rt_video_wait()
{
	while (hal_dma_retired(DMA_CHANNEL) < s_dma_tag)
		rt_kernel_yield();
}

void rt_video_present(uint8_t waitVBlank)
{
	for (uint8_t i = 0; i < waitVBlank; ++i)
	{
		rt_kernel_sig_reset(&s_vblank_signal);
		rt_kernel_sig_wait(&s_vblank_signal);
	}
	hal_video_present();
}
