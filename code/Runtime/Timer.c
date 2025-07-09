/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include <HAL/Timer.h>

#include "Runtime/Timer.h"

uint32_t rt_timer_get_ms() { return hal_timer_get_ms(); }

uint64_t rt_timer_get_cycles() { return hal_timer_get_cycles(); }

void rt_timer_wait_ms(uint32_t ms) { hal_timer_wait_ms(ms); }
