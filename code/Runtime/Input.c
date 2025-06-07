/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include <stdio.h>
#include "Runtime/Input.h"

#include <HAL/I2C.h>
#include <HAL/Timer.h>

#define TRACKBALL_REG_LED_RED 0x00
#define TRACKBALL_REG_LED_GRN 0x01
#define TRACKBALL_REG_LED_BLU 0x02
#define TRACKBALL_REG_LED_WHT 0x03

#define TRACKBALL_REG_LEFT 0x04
#define TRACKBALL_REG_RIGHT 0x05
#define TRACKBALL_REG_UP 0x06
#define TRACKBALL_REG_DOWN 0x07
#define TRACKBALL_REG_SWITCH 0x08
#define TRACKBALL_MSK_SWITCH_STATE 0b10000000

#define TRACKBALL_REG_USER_FLASH 0xD0
#define TRACKBALL_REG_FLASH_PAGE 0xF0
#define TRACKBALL_REG_INT 0xF9
#define TRACKBALL_MSK_INT_TRIGGERED 0b00000001
#define TRACKBALL_MSK_INT_OUT_EN 0b00000010
#define TRACKBALL_REG_CHIP_ID_L 0xFA
#define TRACKBALL_RED_CHIP_ID_H 0xFB
#define TRACKBALL_REG_VERSION 0xFC
#define TRACKBALL_REG_I2C_ADDR 0xFD
#define TRACKBALL_REG_CTRL 0xFE
#define TRACKBALL_MSK_CTRL_SLEEP 0b00000001
#define TRACKBALL_MSK_CTRL_RESET 0b00000010
#define TRACKBALL_MSK_CTRL_FREAD 0b00000100
#define TRACKBALL_MSK_CTRL_FWRITE 0b00001000

static uint8_t s_initialized = 0;

int32_t input_init()
{
	uint8_t data[5];
    for (int32_t i = 0; /*i < 10*/; ++i)
    {
        printf("[Input] Reading trackball chip id (%d)...\n", i);
		hal_i2c_read(0x0a, TRACKBALL_REG_CHIP_ID_L, data, 2);
		if (data[0] == 0x11 && data[1] == 0xba)
        {
			hal_i2c_write(0x0a, TRACKBALL_REG_LED_GRN, 0xff);

			// i2c_read(0x0a, TRACKBALL_REG_INT, data, 1);
			// data[0] |= TRACKBALL_MSK_INT_OUT_EN;
			// i2c_write(0x0a, TRACKBALL_REG_INT, data[0]);

            s_initialized = 1;

            printf("[Input] Trackball initialized successfully.\n");
            return 0;
        }
        hal_timer_wait_ms(100);
    }
    printf("[Input] No trackball found.\n");
    return 1;
}

void input_update()
{
    if (!s_initialized)
        return;

	uint8_t data[5];
	hal_i2c_read(0x0a, TRACKBALL_REG_LEFT, data, 5);
    printf("[Input] %d, %d, %d, %d\n", data[0], data[1], data[2], data[3]);
}
