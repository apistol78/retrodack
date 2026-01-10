/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include "Runtime/I2C.h"
#include "Runtime/Kernel.h"

#include <HAL/I2C.h>

static kernel_cs_t s_i2c_lock;

void rt_i2c_init()
{
	rt_kernel_cs_init(&s_i2c_lock);
}

void rt_i2c_acquire()
{
	rt_kernel_cs_lock(&s_i2c_lock);
}

void rt_i2c_release()
{
	rt_kernel_cs_unlock(&s_i2c_lock);
}

void rt_i2c_write(uint8_t deviceAddr, uint8_t controlAddr, uint8_t controlData, int32_t mode)
{
	const uint32_t tag = hal_i2c_write(deviceAddr, controlAddr, controlData, mode);
	while (hal_i2c_retired() < tag)
		rt_kernel_yield();
}

uint32_t rt_i2c_write_async(uint8_t deviceAddr, uint8_t controlAddr, uint8_t controlData, int32_t mode)
{
	return hal_i2c_write(deviceAddr, controlAddr, controlData, mode);
}

void rt_i2c_read(uint8_t deviceAddr, uint8_t controlAddr, uint8_t* outControlData, uint8_t nbytes, int32_t mode)
{
	const uint32_t tag = hal_i2c_read(deviceAddr, controlAddr, nbytes, mode);
	while (hal_i2c_retired() < tag)
		rt_kernel_yield();

	hal_i2c_read_get(outControlData, nbytes);
}
