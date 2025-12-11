/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include "Runtime/RTC.h"

#include "Runtime/I2C.h"

#define DS231M_ADDR 0x68

void rt_rtc_read(rtc_t* rtc)
{
	uint8_t ds, dm, dh;
	uint8_t cd, cm, cy;

	rt_i2c_read(DS231M_ADDR, 0x00, &ds, 1, RT_I2C_MODE_SLOW);
	rt_i2c_read(DS231M_ADDR, 0x01, &dm, 1, RT_I2C_MODE_SLOW);
	rt_i2c_read(DS231M_ADDR, 0x02, &dh, 1, RT_I2C_MODE_SLOW);

	rt_i2c_read(DS231M_ADDR, 0x03, &cd, 1, RT_I2C_MODE_SLOW);
	rt_i2c_read(DS231M_ADDR, 0x04, &cm, 1, RT_I2C_MODE_SLOW);
	rt_i2c_read(DS231M_ADDR, 0x05, &cy, 1, RT_I2C_MODE_SLOW);

	rtc->year = 2000 + cy;
	rtc->month = cm & 0x1f;
	rtc->mday = cd;

	rtc->hour = dh;
	rtc->minute = dm;
	rtc->second = ds;
}

void rt_rtc_write(rtc_t* rtc)
{
	const uint8_t ds = rtc->second;
	const uint8_t dm = rtc->minute;
	const uint8_t dh = rtc->hour;

	const uint8_t cd = rtc->mday;
	const uint8_t cm = rtc->month;
	const uint8_t cy = rtc->year % 100;

	rt_i2c_write(DS231M_ADDR, 0x00, ds, RT_I2C_MODE_SLOW);
	rt_i2c_write(DS231M_ADDR, 0x01, dm, RT_I2C_MODE_SLOW);
	rt_i2c_write(DS231M_ADDR, 0x02, dh, RT_I2C_MODE_SLOW);

	rt_i2c_write(DS231M_ADDR, 0x03, cd, RT_I2C_MODE_SLOW);
	rt_i2c_write(DS231M_ADDR, 0x04, cm, RT_I2C_MODE_SLOW);
	rt_i2c_write(DS231M_ADDR, 0x05, cy, RT_I2C_MODE_SLOW);
}
