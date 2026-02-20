/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#pragma once

#include <HAL/Common.h>

#define RT_INPUT_BUTTON_A	(1 << 0)
#define RT_INPUT_BUTTON_B	(1 << 1)
#define RT_INPUT_BUTTON_C	(1 << 2)
#define RT_INPUT_BUTTON_D	(1 << 3)
#define RT_INPUT_BUTTON_S1	(1 << 4)
#define RT_INPUT_BUTTON_S2	(1 << 5)
#define RT_INPUT_DPAD_N		(1 << 6)
#define RT_INPUT_DPAD_S		(1 << 7)
#define RT_INPUT_DPAD_E		(1 << 8)
#define RT_INPUT_DPAD_W		(1 << 9)
#define RT_INPUT_TB		    (1 << 10)

#define RT_TB_NONE          0x00000000
#define RT_TB_RED           0x00ff0000
#define RT_TB_GREEN         0x0000ff00
#define RT_TB_BLUE          0x000000ff

typedef struct
{
	uint32_t button;
	uint8_t pressed;
	int32_t x;
	int32_t y;
}
rt_event_t;

EXTERN_C int32_t rt_input_init();

EXTERN_C void rt_input_set_absolute_position(int32_t x, int32_t y);

EXTERN_C void rt_input_get_absolute_position(int32_t* pos);

EXTERN_C void rt_input_get_delta_position(int32_t* pos);

EXTERN_C uint32_t rt_input_get_state();

EXTERN_C uint32_t rt_input_get_event(rt_event_t* ev);

EXTERN_C void rt_input_set_tb_color(uint32_t color);

EXTERN_C void rt_input_show_cursor();

EXTERN_C void rt_input_hide_cursor();

EXTERN_C void rt_input_set_hotspot(int32_t x, int32_t y);