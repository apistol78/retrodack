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
#include <HAL/Sprite.h>
#include <HAL/Timer.h>
#include <HAL/Video.h>

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

static int32_t s_filteredDeltaX = 0;
static int32_t s_filteredDeltaY = 0;
static int32_t s_absX = 0;
static int32_t s_absY = 0;
static int32_t s_deltaX = 0;
static int32_t s_deltaY = 0;
static uint32_t s_pressed = 0;
static int32_t s_hotX = 0;
static int32_t s_hotY = 0;

#define TB_SPEED	5
#define NEVENTS		128

static rt_event_t s_events[NEVENTS];
static int32_t s_events_in = 0;
static int32_t s_events_out = 0;

static void tb_input_interrupt(uint32_t source)
{
	hal_i2c_write(0x0a, TRACKBALL_REG_INT, 0);

	uint8_t data[5] = { 0, 0, 0, 0, 0 };
	hal_i2c_read(0x0a, TRACKBALL_REG_LEFT, data, 5);

	#define TB_DATA(N) \
		(int32_t)((data[N] > 1) ? (data[N] * TB_SPEED) : data[N])

	const int32_t dx = TB_DATA(0) - TB_DATA(1);
	const int32_t dy = TB_DATA(2) - TB_DATA(3);

	const int32_t f0 = 64;
	s_filteredDeltaX = (dx * f0 + s_filteredDeltaX * (255 - f0)) / 256;
	s_filteredDeltaY = (dy * f0 + s_filteredDeltaY * (255 - f0)) / 256;

	s_absX += s_filteredDeltaX;
	s_absY += s_filteredDeltaY;

	s_deltaX += s_filteredDeltaX;
	s_deltaY += s_filteredDeltaY;

	const int32_t f1 = 240;
	s_filteredDeltaX = (s_filteredDeltaX * f1) / 256;
	s_filteredDeltaY = (s_filteredDeltaY * f1) / 256;

	// Clamp absolute position to the size of the current resolution.
	const int32_t width = hal_video_get_resolution_width();
	const int32_t height = hal_video_get_resolution_height();

	if (s_absX < 0)
		s_absX = 0;
	else if (s_absX > width - 1)
		s_absX = width - 1;

	if (s_absY < 0)
		s_absY = 0;
	else if (s_absY > height - 1)
		s_absY = height - 1;

	// Place position of first sprite as a mouse cursor;
	// offset to ensure center of sprite is a mouse position.
	hal_sprite_set_position(0, s_absX - s_hotX, s_absY - s_hotY);

	if (data[0] || data[1] || data[2] || data[3])
	{
		rt_event_t* ev = &s_events[s_events_in];
		ev->button = 0;
		ev->pressed = 0;
		ev->x = s_absX;
		ev->y = s_absY;
		s_events_in = (s_events_in + 1) & (NEVENTS - 1);
		if (s_events_in == s_events_out)
			s_events_out = (s_events_out + 1) & (NEVENTS - 1);		
	}

	const uint32_t pressed = s_pressed;

	if (data[4])
		s_pressed |= RT_INPUT_TB;
	else
		s_pressed &= ~RT_INPUT_TB;

	if (pressed != s_pressed)
	{
		rt_event_t* ev = &s_events[s_events_in];
		ev->button = RT_INPUT_TB;
		ev->pressed = (RT_INPUT_TB & s_pressed) ? 1 : 0;
		ev->x = s_absX;
		ev->y = s_absY;
		s_events_in = (s_events_in + 1) & (NEVENTS - 1);
		if (s_events_in == s_events_out)
			s_events_out = (s_events_out + 1) & (NEVENTS - 1);
	}

	hal_i2c_write(0x0a, TRACKBALL_REG_INT, TRACKBALL_MSK_INT_OUT_EN);
}

static void gpio_input_interrupt(uint32_t source)
{
	uint16_t data = 0;
	hal_i2c_read(0x20, 0x00, (uint8_t*)&data, 2);
	data = ~data;

	#define S(bit, mask) \
		if (data & bit) { s_pressed |= mask; } else { s_pressed &= ~mask; }

	const uint32_t pressed = s_pressed;
	S(0x0020, RT_INPUT_DPAD_S);
	S(0x0040, RT_INPUT_DPAD_E);
	S(0x0080, RT_INPUT_DPAD_W);
	S(0x0010, RT_INPUT_DPAD_N);
	S(0x0100, RT_INPUT_BUTTON_S1);
	S(0x0200, RT_INPUT_BUTTON_S2);
	S(0x0001, RT_INPUT_BUTTON_D);
	S(0x0008, RT_INPUT_BUTTON_A);
	S(0x0004, RT_INPUT_BUTTON_B);
	S(0x0002, RT_INPUT_BUTTON_C);

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
	for (int32_t i = 0; i < 10; ++i)
	{
		hal_i2c_read(0x0a, TRACKBALL_REG_CHIP_ID_L, data, 2);
		if (data[0] == 0x11 && data[1] == 0xba)
		{
			found = 1;
			break;
		}
		hal_timer_wait_ms(100);
	}

	// Setup trackball.
	// if (found)
	{
		hal_interrupt_set_handler(IRQ_SOURCE_PLIC_0, tb_input_interrupt);

		// Turn on green backlight to indicate success.
		hal_i2c_write(0x0a, TRACKBALL_REG_LED_GRN, 0xff);

		// Enable interrupt pin; notify CPU everytime
		// track ball position change.
		hal_i2c_write(0x0a, TRACKBALL_REG_INT, TRACKBALL_MSK_INT_OUT_EN);

		// Read data from TB; to ensure interrupt state
		// in TB is reset.
		for (int i = 0; i < 100; ++i)
		{
			uint8_t data[5] = { 0, 0, 0, 0, 0 };
			hal_i2c_read(0x0a, TRACKBALL_REG_LEFT, data, 5);
			hal_timer_wait_ms(10);
		}
	}

	// Setup button inputs.
	hal_interrupt_set_handler(IRQ_SOURCE_PLIC_1, gpio_input_interrupt);
	return 0;
}

void rt_input_set_absolute_position(int32_t x, int32_t y)
{
	s_absX = x;
	s_absY = y;
	hal_sprite_set_position(0, s_absX - s_hotX, s_absY - s_hotY);
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
	// uint32_t RA;
	// __asm__ volatile (
	// 	"mv %0, ra	\n"
	// 	: "=r" (RA)
	// 	:
	// );

	// printf("rt_input_get_event\n");
	// printf("s_events %p\n", s_events);
	// printf("s_events_in %d\n", s_events_in);
	// printf("s_events_out %d\n", s_events_out);
	// printf("ev %p\n", ev);
	// printf("RA %08x\n", RA);

	if (s_events_in == s_events_out)
		return 0;

	memcpy(ev, &s_events[s_events_out], sizeof(rt_event_t));
	s_events_out = (s_events_out + 1) & (NEVENTS - 1);

	return 1;
}

void rt_input_set_tb_color(int32_t clr)
{
	switch (clr)
	{
	case RT_TB_RED:
		hal_i2c_write(0x0a, TRACKBALL_REG_LED_RED, 0xff);
		hal_i2c_write(0x0a, TRACKBALL_REG_LED_GRN, 0x00);
		hal_i2c_write(0x0a, TRACKBALL_REG_LED_BLU, 0x00);
		break;
	case RT_TB_GREEN:
		hal_i2c_write(0x0a, TRACKBALL_REG_LED_RED, 0x00);
		hal_i2c_write(0x0a, TRACKBALL_REG_LED_GRN, 0xff);
		hal_i2c_write(0x0a, TRACKBALL_REG_LED_BLU, 0x00);
		break;
	case RT_TB_BLUE:
		hal_i2c_write(0x0a, TRACKBALL_REG_LED_RED, 0x00);
		hal_i2c_write(0x0a, TRACKBALL_REG_LED_GRN, 0x00);
		hal_i2c_write(0x0a, TRACKBALL_REG_LED_BLU, 0xff);
		break;
	default:
		hal_i2c_write(0x0a, TRACKBALL_REG_LED_RED, 0x00);
		hal_i2c_write(0x0a, TRACKBALL_REG_LED_GRN, 0x00);
		hal_i2c_write(0x0a, TRACKBALL_REG_LED_BLU, 0x00);
		break;
	}
}

void rt_input_set_hotspot(int32_t x, int32_t y)
{
	s_hotX = x;
	s_hotY = y;
	hal_sprite_set_position(0, s_absX - s_hotX, s_absY - s_hotY);
}
