/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include <stdlib.h>
#include <string.h>

#include "Runtime/Video.h"

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

void rt_video_clear(uint8_t idx)
{
	const uint32_t pixels = hal_video_get_resolution_width() * hal_video_get_resolution_height();
	uint8_t* framebuffer = (uint8_t*)hal_video_get_secondary_target();
	memset(framebuffer, idx, pixels);
}

void rt_video_blit(const void* source)
{
	const uint32_t pixels = hal_video_get_resolution_width() * hal_video_get_resolution_height();
	uint8_t* target = (uint8_t*)hal_video_get_secondary_target();
	memcpy(target, source, pixels);
}
