/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#pragma once

#include <HAL/Common.h>

typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t mday;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
}
rtc_t;

EXTERN_C void rt_rtc_read(rtc_t* rtc);

EXTERN_C void rt_rtc_write(rtc_t* rtc);

