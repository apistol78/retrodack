/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#pragma once

#include <HAL/Common.h>

EXTERN_C int32_t rt_audio_init();

EXTERN_C void rt_audio_set_volume(uint8_t volume);

EXTERN_C void rt_audio_set_playback_rate(uint32_t rate);

EXTERN_C uint32_t rt_audio_get_queued();

EXTERN_C void rt_audio_play_stereo(const void* samples, uint32_t nsamples);

EXTERN_C void rt_audio_wait();
