/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#pragma once

#include <HAL/Common.h>

#define RT_AUDIO_MODE_APPEND	0x00000000
#define RT_AUDIO_MODE_REPLACE   0x10000000
#define RT_AUDIO_MODE_MONO		0x00000000
#define RT_AUDIO_MODE_STEREO	0x20000000

EXTERN_C int32_t rt_audio_init();

EXTERN_C void rt_audio_set_volume(uint8_t volume);

EXTERN_C void rt_audio_set_playback_rate(uint32_t rate);

EXTERN_C void rt_audio_set_filter(uint8_t filter);

EXTERN_C uint8_t rt_audio_get_num_channels();

EXTERN_C uint8_t rt_audio_is_channels_busy(uint32_t channel_mask);

EXTERN_C void rt_audio_play(uint8_t channel, const void* samples, uint32_t nsamples, uint32_t mode);

EXTERN_C void rt_audio_set_channel_volume(uint8_t channel, uint8_t volume);

EXTERN_C void rt_audio_wait(uint32_t channel_mask);

EXTERN_C int32_t rt_audio_headphones_connected();
