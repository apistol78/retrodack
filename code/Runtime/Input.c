/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include <stdio.h>
#include <string.h>
#include "Runtime/Cursor.h"
#include "Runtime/I2C.h"
#include "Runtime/Input.h"
#include "Runtime/Kernel.h"
#include "Runtime/Video.h"

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

static float s_filteredDeltaX = 0;
static float s_filteredDeltaY = 0;
static float s_absX = 0;
static float s_absY = 0;
static float s_deltaX = 0;
static float s_deltaY = 0;
static int32_t s_lastAbsX = 0;
static int32_t s_lastAbsY = 0;

static uint32_t s_pressed = 0;
static int32_t s_hotX = 0;
static int32_t s_hotY = 0;

#define NEVENTS 256

static rt_event_t s_events[NEVENTS];
static int32_t s_events_in = 0;
static int32_t s_events_out = 0;

static volatile kernel_cs_t s_input_lock;
static volatile kernel_sig_t s_input_signal;

static void input_interrupt(uint32_t source)
{
	rt_kernel_sig_raise(&s_input_signal);
}

static float max(float a, float b)
{
	return (a > b) ? a : b;
}

static float min(float a, float b)
{
	return (a < b) ? a : b;
}

static float s_abs(float a)
{
	return a >= 0.0f ? a : -a;
}

static void input_thread()
{
	int32_t absX, absY;
	int32_t wait = 100;

	for (;;)
	{
		// Ensure TB emit interrupt; seems unreliable.
		rt_i2c_acquire();
		rt_i2c_write(0x0a, TRACKBALL_REG_INT, 0, RT_I2C_MODE_FAST);
		rt_i2c_write(0x0a, TRACKBALL_REG_INT, TRACKBALL_MSK_INT_OUT_EN, RT_I2C_MODE_FAST);
		rt_i2c_release();

		// Wait for signal.
		rt_kernel_sig_try_wait(&s_input_signal, wait);

		// Read devices.
		rt_i2c_acquire();

		// Trackball
		{
			uint8_t data[5] = { 0, 0, 0, 0, 0 };
			rt_i2c_read(0x0a, TRACKBALL_REG_LEFT, data, 5, RT_I2C_MODE_FAST);

			#define TB_DATA(N) ((int32_t)data[N])

			const int32_t dx = TB_DATA(0) - TB_DATA(1);
			const int32_t dy = TB_DATA(2) - TB_DATA(3);

			const float fdx = (float)dx;
			const float fdy = (float)dy;

			const float f0 = 0.1f;
			s_filteredDeltaX = fdx * f0 + s_filteredDeltaX * (1.0f - f0);
			s_filteredDeltaY = fdy * f0 + s_filteredDeltaY * (1.0f - f0);

			float fm = 0.0f;
			fm = max(fm, s_abs(s_filteredDeltaX));
			fm = max(fm, s_abs(s_filteredDeltaY));
			fm = min(fm, 1.0f);

			const float f1 = fm * 10.0f + (1.0f - fm) * 4.0f;
			s_absX += s_filteredDeltaX * f1;
			s_absY += s_filteredDeltaY * f1;
			s_deltaX += s_filteredDeltaX * f1;
			s_deltaY += s_filteredDeltaY * f1;

			const float f2 = 0.925f;
			s_filteredDeltaX *= f2;
			s_filteredDeltaY *= f2;

			// Calculate wait time until next read based on filtered movement,
			// if completely stopped then we can wait longer for an interrupt.            
			if (fm < 0.01f)
			{
				s_filteredDeltaX = 0.0f;
				s_filteredDeltaY = 0.0f;
				wait = 100;
			}
			else
				wait = 10;

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

			// Convert to integer.
			absX = (int32_t)s_absX;
			absY = (int32_t)s_absY;

			// Place position of first sprite as a mouse cursor;
			// offset to ensure center of sprite is a mouse position.
			hal_sprite_set_position(0, absX - s_hotX, absY - s_hotY);
		
			if (absX != s_lastAbsX || absY != s_lastAbsY)
			{
				rt_kernel_cs_lock(&s_input_lock);

				rt_event_t* ev = &s_events[s_events_in];
				ev->button = 0;
				ev->pressed = 0;
				ev->x = absX;
				ev->y = absY;
				s_events_in = (s_events_in + 1) & (NEVENTS - 1);
				if (s_events_in == s_events_out)
					s_events_out = (s_events_out + 1) & (NEVENTS - 1);

				rt_kernel_cs_unlock(&s_input_lock);

				s_lastAbsX = absX;
				s_lastAbsY = absY;
			}

			const uint32_t pressed = s_pressed;

			if (data[4])
				s_pressed |= RT_INPUT_TB;
			else
				s_pressed &= ~RT_INPUT_TB;

			if (pressed != s_pressed)
			{
				rt_kernel_cs_lock(&s_input_lock);

				rt_event_t* ev = &s_events[s_events_in];
				ev->button = RT_INPUT_TB;
				ev->pressed = (RT_INPUT_TB & s_pressed) ? 1 : 0;
				ev->x = absX;
				ev->y = absY;
				s_events_in = (s_events_in + 1) & (NEVENTS - 1);
				if (s_events_in == s_events_out)
					s_events_out = (s_events_out + 1) & (NEVENTS - 1);

				rt_kernel_cs_unlock(&s_input_lock);
			}
		}

		// Buttons
		{
			uint16_t data = 0;
			rt_i2c_read(0x20, 0x00, (uint8_t*)&data, 2, RT_I2C_MODE_FAST);
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
						rt_kernel_cs_lock(&s_input_lock);

						rt_event_t* ev = &s_events[s_events_in];
						ev->button = btn;
						ev->pressed = (btn & s_pressed) ? 1 : 0;
						ev->x = absX;
						ev->y = absY;
						s_events_in = (s_events_in + 1) & (NEVENTS - 1);
						if (s_events_in == s_events_out)
							s_events_out = (s_events_out + 1) & (NEVENTS - 1);

						rt_kernel_cs_unlock(&s_input_lock);
					}
				}
			}
		}

		rt_i2c_release();
	}
}

int32_t rt_input_init()
{
	uint8_t data[5] = { 0, 0, 0, 0, 0 };
	uint8_t found = 0;

	// Ensure everything is reset.
	s_filteredDeltaX = 0.0f;
	s_filteredDeltaY = 0.0f;
	s_absX = 0.0f;
	s_absY = 0.0f;
	s_deltaX = 0.0f;
	s_deltaY = 0.0f;
	s_lastAbsX = 0;
	s_lastAbsY = 0;
	s_pressed = 0;
	s_hotX = 0;
	s_hotY = 0;
	s_events_in = 0;
	s_events_out = 0;

	// Locate trackball by reading it's identification.
	for (int32_t i = 0; i < 10; ++i)
	{
		rt_i2c_read(0x0a, TRACKBALL_REG_CHIP_ID_L, data, 2, RT_I2C_MODE_FAST);
		if (data[0] == 0x11 && data[1] == 0xba)
		{
			found = 1;
			break;
		}
		hal_timer_wait_ms(100);
	}

	// Create input queue thread.
	rt_kernel_cs_init(&s_input_lock);
	rt_kernel_sig_init(&s_input_signal);
	rt_kernel_create_thread(input_thread, "input");

	// Setup trackball.
	// if (found)
	{
		// Turn on green backlight to indicate success.
		rt_i2c_write(0x0a, TRACKBALL_REG_LED_GRN, 0xff, RT_I2C_MODE_FAST);

		// Enable interrupt pin; notify CPU everytime
		// track ball position change.
		rt_i2c_write(0x0a, TRACKBALL_REG_INT, TRACKBALL_MSK_INT_OUT_EN, RT_I2C_MODE_FAST);

		// Read data from TB; to ensure interrupt state
		// in TB is reset.
		for (int i = 0; i < 100; ++i)
		{
			uint8_t data[5] = { 0, 0, 0, 0, 0 };
			rt_i2c_read(0x0a, TRACKBALL_REG_LEFT, data, 5, RT_I2C_MODE_FAST);
			hal_timer_wait_ms(10);
		}
	}

	// Setup interrupt handler.
	hal_interrupt_set_handler(IRQ_SOURCE_PLIC_0, input_interrupt);

	// Setup default cursor.
	hal_sprite_set_visible(0, 0xff);
	hal_sprite_set_bits(0, c_mouseCursor, 32, 32);
	rt_video_set_palette(2, 0xffffff);
	rt_video_set_palette(3, 0x000000);
	rt_input_set_hotspot(16, 16);
	
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
	pos[0] = (int32_t)s_absX;
	pos[1] = (int32_t)s_absY;
}

void rt_input_get_delta_position(int32_t* pos)
{
	pos[0] = (int32_t)s_deltaX;
	pos[1] = (int32_t)s_deltaY;
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

	rt_kernel_cs_lock(&s_input_lock);

	memcpy(ev, &s_events[s_events_out], sizeof(rt_event_t));
	s_events_out = (s_events_out + 1) & (NEVENTS - 1);

	rt_kernel_cs_unlock(&s_input_lock);
	return 1;
}

void rt_input_set_tb_color(int32_t clr)
{
	switch (clr)
	{
	case RT_TB_RED:
		rt_i2c_write_async(0x0a, TRACKBALL_REG_LED_RED, 0xff, RT_I2C_MODE_FAST);
		rt_i2c_write_async(0x0a, TRACKBALL_REG_LED_GRN, 0x00, RT_I2C_MODE_FAST);
		rt_i2c_write_async(0x0a, TRACKBALL_REG_LED_BLU, 0x00, RT_I2C_MODE_FAST);
		break;
	case RT_TB_GREEN:
		rt_i2c_write_async(0x0a, TRACKBALL_REG_LED_RED, 0x00, RT_I2C_MODE_FAST);
		rt_i2c_write_async(0x0a, TRACKBALL_REG_LED_GRN, 0xff, RT_I2C_MODE_FAST);
		rt_i2c_write_async(0x0a, TRACKBALL_REG_LED_BLU, 0x00, RT_I2C_MODE_FAST);
		break;
	case RT_TB_BLUE:
		rt_i2c_write_async(0x0a, TRACKBALL_REG_LED_RED, 0x00, RT_I2C_MODE_FAST);
		rt_i2c_write_async(0x0a, TRACKBALL_REG_LED_GRN, 0x00, RT_I2C_MODE_FAST);
		rt_i2c_write_async(0x0a, TRACKBALL_REG_LED_BLU, 0xff, RT_I2C_MODE_FAST);
		break;
	default:
		rt_i2c_write_async(0x0a, TRACKBALL_REG_LED_RED, 0x00, RT_I2C_MODE_FAST);
		rt_i2c_write_async(0x0a, TRACKBALL_REG_LED_GRN, 0x00, RT_I2C_MODE_FAST);
		rt_i2c_write_async(0x0a, TRACKBALL_REG_LED_BLU, 0x00, RT_I2C_MODE_FAST);
		break;
	}
}

void rt_input_set_hotspot(int32_t x, int32_t y)
{
	s_hotX = x;
	s_hotY = y;
	hal_sprite_set_position(0, s_absX - s_hotX, s_absY - s_hotY);
}
