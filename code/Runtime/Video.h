/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#pragma once

#include <HAL/Video.h>

inline int32_t rt_video_init() { return hal_video_init(); }

inline int32_t rt_video_set_mode(int32_t mode) { return hal_video_set_mode(mode); }

EXTERN_C void* rt_video_create_target();

EXTERN_C void rt_video_destroy_target(void* target);

inline int32_t rt_video_get_resolution_width() { return hal_video_get_resolution_width(); }

inline int32_t rt_video_get_resolution_height() { return hal_video_get_resolution_height(); }

inline void rt_video_set_palette(uint8_t index, uint32_t color) { return hal_video_set_palette(index, color); }

inline void* rt_video_get_primary_target() { return hal_video_get_primary_target(); }

inline void* rt_video_get_secondary_target() { return hal_video_get_secondary_target(); }

EXTERN_C void rt_video_clear(uint8_t idx);

EXTERN_C void rt_video_blit(const void* source);

inline void rt_video_present() { hal_video_present(); }
