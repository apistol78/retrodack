/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "Runtime/Runtime.h"

// C64 style font.
#include "Runtime/font8x8_c64.h"

#define FW 360
#define FH 360

#define CC 40
#define CR 40

const int32_t offset_y = (FW - (CC * 8)) / 2;
const int32_t offset_x = (FH - (CR * 8)) / 2;

uint32_t s_thread = 0;
kernel_sig_t s_redraw;
kernel_sig_t s_redrawn;
char s_cbuffer[CC * CR];
int s_x = 0;
int s_y = 0;
int s_cursor = 1;
int s_stateOfCharge = 0;

static void rt_console_draw_console()
{
	rt_gfx_context_t ctx;
	ctx.width = FW;
	ctx.height = FH;
	ctx.pixels = (uint8_t*)rt_video_get_secondary_target();

	rt_video_clear(0);
	rt_video_wait();

	// Console characters,
	for (int32_t y = 0; y < CR; ++y)
	{
		for (int32_t x = 0; x < CC; ++x)
		{
			const char ch = s_cbuffer[x + y * CC];
			if (ch >= ' ')
				rt_gfx_draw_char(&ctx, font8x8_c64, offset_x + x * 8, offset_y + y * 8, ch, 1);
		}
	}

	// Blinking cursor.
	if (s_cursor)
		rt_gfx_fill_rect(&ctx, s_x * 8 + offset_x, s_y * 8 + offset_y, 7, 8, 1);

	// Battery indicator.
	rt_gfx_fill_rect(&ctx, FW - 30, 10, (s_stateOfCharge * 20) / 100, 10, 1);
	rt_gfx_draw_rect(&ctx, FW - 30, 10, 20, 10, 1);

	rt_video_present(1);
}

static void rt_console_putchar(char c)
{
	if (c == '\n')
	{
		s_x = 0;
		if (s_y < (CR - 1))
			s_y++;
		else
		{
			for (int32_t i = 0; i < (CR - 1); ++i)
				memmove(&s_cbuffer[i * CC], &s_cbuffer[(i + 1) * CC], CC);
			memset(&s_cbuffer[(CR - 1) * CC], 0, CC);
		}
	}
    else if (c == '\r')
    {
        s_x = 0;
    }
	else if (c == '\b')
	{
		if (s_x > 0)
		{
			s_x--;
			s_cbuffer[s_x + s_y * CC] = 0;
		}
	}
	else
    {
		if (s_x < CC && s_y < CR) {
			s_cbuffer[s_x + s_y * CC] = c;
		}
		s_x++;
	}
}

static void rt_console_thread_redraw()
{
	uint32_t last = 0;
	battery_t bat;

	// Read initial battery status.
	rt_battery_read(&bat);

	// Redraw console async.
	for (;;)
	{
		rt_kernel_sig_try_wait(&s_redraw, 200);

		const uint32_t ms = rt_timer_get_ms();
		if ((ms - last) > 1000)
		{
			rt_battery_read(&bat);
			s_stateOfCharge = bat.stageOfCharge;
			last = ms;
		}

		rt_console_draw_console();
		s_cursor = 1 - s_cursor;
		rt_kernel_sig_raise(&s_redrawn);
	}	
}

// public

void rt_console_init()
{
	rt_video_set_mode(VMODE_360_360_8);

	rt_video_set_palette(0, 0x5e3d00);	// background
  	rt_video_set_palette(1, 0xffffff);	// foreground

	rt_video_clear(0);
	rt_video_wait();

	rt_kernel_sig_init(&s_redraw);
	rt_kernel_sig_init(&s_redrawn);

	s_thread = rt_kernel_create_thread(rt_console_thread_redraw, "console");
}

void rt_console_shutdown()
{
	rt_kernel_destroy_thread(s_thread);
	rt_video_clear(0);
	rt_video_wait();
}

void rt_console_clear()
{
	memset(s_cbuffer, 0, CC * CR);
	s_x = 0;
	s_y = 0;
	rt_kernel_sig_raise(&s_redraw);
}

void rt_console_putc(char c)
{
	rt_console_putchar(c);
	rt_kernel_sig_raise(&s_redraw);
}

void rt_console_print(const char* str)
{
	for (const char* c = str; *c != 0; ++c)
        rt_console_putchar(*c);
	rt_kernel_sig_raise(&s_redraw);
}

void rt_console_printf(const char* str, ...)
{
	char buf[128];
	va_list args;
	va_start(args, str);
	vsnprintf(buf, sizeof(buf), str, args);
	va_end(args);
	rt_console_print(buf);
}

void rt_console_flush()
{
	rt_kernel_sig_reset(&s_redrawn);
	rt_kernel_sig_raise(&s_redraw);
	rt_kernel_sig_wait(&s_redrawn);
}
