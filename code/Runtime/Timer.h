/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#pragma once

#include <HAL/Timer.h>

EXTERN_C void rt_timer_init();

EXTERN_C uint32_t rt_timer_get_ms();

EXTERN_C uint64_t rt_timer_get_cycles();

EXTERN_C void rt_timer_wait_ms(uint32_t ms);
