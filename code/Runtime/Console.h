/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#pragma once

#include <HAL/Common.h>

EXTERN_C void fb_init();

EXTERN_C void fb_shutdown();

EXTERN_C void fb_clear();

EXTERN_C void fb_putc(char c);

EXTERN_C void fb_print(const char* str);

EXTERN_C void fb_printf(const char* str, ...);
