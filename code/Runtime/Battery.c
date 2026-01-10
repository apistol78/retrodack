/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include "Runtime/Battery.h"

#include "Runtime/I2C.h"
#include "Runtime/Kernel.h"

#define BQ27441_ADDR 0x55

void rt_battery_read(battery_t* bat)
{
    uint8_t data[2] = { 0, 0 };

    rt_i2c_acquire();

    rt_i2c_read(BQ27441_ADDR, 0x04, data, 2, RT_I2C_MODE_SLOW);
    bat->voltage = (((uint32_t)data[1]) << 8) | data[0];

    rt_i2c_read(BQ27441_ADDR, 0x0a, data, 2, RT_I2C_MODE_SLOW);
    bat->fullAvailableCapacity = (((uint32_t)data[1]) << 8) | data[0];

    rt_i2c_read(BQ27441_ADDR, 0x0e, data, 2, RT_I2C_MODE_SLOW);
    bat->fullChargeCapacity = (((uint32_t)data[1]) << 8) | data[0];

    rt_i2c_read(BQ27441_ADDR, 0x0c, data, 2, RT_I2C_MODE_SLOW);
    bat->remainingCapacity = (((uint32_t)data[1]) << 8) | data[0];

    rt_i2c_read(BQ27441_ADDR, 0x10, data, 2, RT_I2C_MODE_SLOW);
    bat->current = (((uint32_t)data[1]) << 8) | data[0];

    rt_i2c_read(BQ27441_ADDR, 0x18, data, 2, RT_I2C_MODE_SLOW);
    bat->power = (((uint32_t)data[1]) << 8) | data[0];

    rt_i2c_read(BQ27441_ADDR, 0x1c, data, 2, RT_I2C_MODE_SLOW);
    bat->stageOfCharge = (((uint32_t)data[1]) << 8) | data[0];

    rt_i2c_release();
}
