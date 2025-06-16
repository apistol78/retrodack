/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#pragma once

#include <HAL/Timer.h>

inline uint32_t rt_timer_get_ms() { return hal_timer_get_ms(); }

inline uint64_t rt_timer_get_cycles() { return hal_timer_get_cycles(); }

inline void rt_timer_wait_ms(uint32_t ms) { hal_timer_wait_ms(ms); }
