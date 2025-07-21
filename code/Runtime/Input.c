/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include <stdio.h>
#include <string.h>
#include "Runtime/Input.h"

#include <HAL/I2C.h>
#include <HAL/Interrupt.h>
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

static int32_t s_absX = 0;
static int32_t s_absY = 0;
static int32_t s_deltaX = 0;
static int32_t s_deltaY = 0;
static uint32_t s_pressed = 0;

#define NEVENTS 128

static rt_event_t s_events[NEVENTS];
static int32_t s_events_in = 0;
static int32_t s_events_out = 0;

static void tb_input_interrupt(uint32_t source)
{
	uint8_t data[5] = { 0, 0, 0, 0, 0 };
	hal_i2c_read(0x0a, TRACKBALL_REG_LEFT, data, 5);

	s_absX -= data[0];
	s_absX += data[1];
	s_absY -= data[2];
	s_absY += data[3];

	if (s_absX < 0)
		s_absX = 0;
	else if (s_absX > 719)
		s_absX = 719;

	if (s_absY < 0)
		s_absY = 0;
	else if (s_absY > 719)
		s_absY = 719;

	s_deltaX -= data[0];
	s_deltaX += data[1];
	s_deltaY -= data[2];
	s_deltaY += data[3];

	if (data[4])
		s_pressed |= RT_INPUT_TB;
	else
		s_pressed &= ~RT_INPUT_TB;
}

static void gpio_input_interrupt(uint32_t source)
{
	uint16_t data = 0;
	hal_i2c_read(0x20, 0x00, (uint8_t*)&data, 2);
	data = ~data;

	#define S(bit, mask) \
		if (data & bit) { s_pressed |= mask; } else { s_pressed &= ~mask; }

	const uint32_t pressed = s_pressed;
	S(0x0020, RT_INPUT_BUTTON_A);
	S(0x0040, RT_INPUT_BUTTON_B);
	S(0x0080, RT_INPUT_BUTTON_C);
	S(0x0010, RT_INPUT_BUTTON_D);
	S(0x0100, RT_INPUT_BUTTON_S1);
	S(0x0200, RT_INPUT_BUTTON_S2);
	S(0x0001, RT_INPUT_DPAD_N);
	S(0x0008, RT_INPUT_DPAD_S);
	S(0x0004, RT_INPUT_DPAD_E);
	S(0x0002, RT_INPUT_DPAD_W);

	#undef S

	if (pressed != s_pressed)
	{
		const uint32_t m = s_pressed ^ pressed;
		for (int32_t i = 0; i < 11; ++i)
		{
			const uint32_t btn = 1 << i;
			if ((btn & m) != 0)
			{
				rt_event_t* ev = &s_events[s_events_in];
				ev->button = btn;
				ev->pressed = (btn & s_pressed) ? 1 : 0;
				ev->x = s_absX;
				ev->y = s_absY;
				s_events_in = (s_events_in + 1) & (NEVENTS - 1);
				if (s_events_in == s_events_out)
					s_events_out = (s_events_out + 1) & (NEVENTS - 1);
			}
		}
	}
}

int32_t rt_input_init()
{
	uint8_t data[5] = { 0, 0, 0, 0, 0 };
	uint8_t found = 0;

	// Locate trackball by reading it's identification.
	for (int32_t i = 0; i < 4; ++i)
	{
		printf("[Input] Reading trackball chip id (%d)...\n", i);
		hal_i2c_read(0x0a, TRACKBALL_REG_CHIP_ID_L, data, 2);
		if (data[0] == 0x11 && data[1] == 0xba)
		{
			found = 1;
			break;
		}
		hal_timer_wait_ms(100);
	}

	// Setup trackball.
	if (found)
	{
		hal_interrupt_set_handler(IRQ_SOURCE_PLIC_0, tb_input_interrupt);

		// Enable interrupt pin; notify CPU everytime
		// track ball position change.
		hal_i2c_read(0x0a, TRACKBALL_REG_INT, data, 1);
		data[0] |= TRACKBALL_MSK_INT_OUT_EN;
		hal_i2c_write(0x0a, TRACKBALL_REG_INT, data[0]);

		// Turn on green backlight to indicate success.
		hal_i2c_write(0x0a, TRACKBALL_REG_LED_GRN, 0xff);
	}
	else
		printf("[Input] No trackball found.\n");

	// Setup button inputs.
	hal_interrupt_set_handler(IRQ_SOURCE_PLIC_1, gpio_input_interrupt);

	printf("[Input] Initialized successfully.\n");
	return 0;
}

void rt_input_get_absolute_position(int32_t* pos)
{
	pos[0] = s_absX;
	pos[1] = s_absY;
}

void rt_input_get_delta_position(int32_t* pos)
{
	pos[0] = s_deltaX;
	pos[1] = s_deltaY;
	s_deltaX = 0;
	s_deltaY = 0;
}

uint32_t rt_input_get_state()
{
	return s_pressed;
}

uint32_t rt_input_get_event(rt_event_t* ev)
{
	if (s_events_in == s_events_out)
		return 0;

	memcpy(ev, &s_events[s_events_out], sizeof(rt_event_t));
	s_events_out = (s_events_out + 1) & (NEVENTS - 1);

	return 1;
}
