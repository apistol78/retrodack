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
	uint32_t voltage;					// mV
	uint32_t fullAvailableCapacity;		// mAh
	uint32_t fullChargeCapacity;		// mAh
	uint32_t remainingCapacity;			// mAh
	uint32_t current;					// mA
	uint32_t power;						// mW
	uint32_t stageOfCharge;				// %
}
battery_t;

EXTERN_C void rt_battery_init();

EXTERN_C void rt_battery_read(battery_t* bat);
