/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#pragma once

#include <HAL/Common.h>

#define RT_I2C_MODE_SLOW	0
#define RT_I2C_MODE_FAST	1

EXTERN_C void rt_i2c_write(uint8_t deviceAddr, uint8_t controlAddr, uint8_t controlData, int32_t mode);

EXTERN_C uint32_t rt_i2c_write_async(uint8_t deviceAddr, uint8_t controlAddr, uint8_t controlData, int32_t mode);

EXTERN_C void rt_i2c_read(uint8_t deviceAddr, uint8_t controlAddr, uint8_t* outControlData, uint8_t nbytes, int32_t mode);
