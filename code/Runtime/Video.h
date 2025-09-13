/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#pragma once

#include <HAL/Video.h>

EXTERN_C int32_t rt_video_init();

EXTERN_C int32_t rt_video_set_mode(int32_t mode);

EXTERN_C void* rt_video_create_target();

EXTERN_C void rt_video_destroy_target(void* target);

EXTERN_C int32_t rt_video_get_resolution_width();

EXTERN_C int32_t rt_video_get_resolution_height();

EXTERN_C void rt_video_set_palette(uint8_t index, uint32_t color);

EXTERN_C void* rt_video_get_primary_target();

EXTERN_C void* rt_video_get_secondary_target();

EXTERN_C void rt_video_clear(uint8_t idx);

EXTERN_C void rt_video_blit(const void* source);

EXTERN_C void rt_video_wait();

EXTERN_C void rt_video_present(uint8_t waitVBlank);
