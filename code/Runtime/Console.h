/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#pragma once

#include <HAL/Common.h>

EXTERN_C void rt_console_init();

EXTERN_C void rt_console_shutdown();

EXTERN_C void rt_console_clear();

EXTERN_C void rt_console_putc(char c);

EXTERN_C void rt_console_print(const char* str);

EXTERN_C void rt_console_printf(const char* str, ...);
