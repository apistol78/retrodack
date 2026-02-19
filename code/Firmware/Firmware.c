/*
 RetroDÄCK
 Copyright (c) 2025-2026 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <HAL/Interrupt.h>
#include <HAL/SD.h>
#include <HAL/Timer.h>
#include <HAL/UART.h>
#include <HAL/Sprite.h>

#include "Runtime/Runtime.h"
#include "Runtime/font8x8_c64.h"

#include "Firmware/Palette.h"
#include "Firmware/Remote.h"

// #include "Runtime/USB/Max3420.h"
// #include "Runtime/USB/UsbMassStorage.h"
// #include "Runtime/USB/ScsiCommands.h"

void __register_exitproc(void) {}
void __call_exitprocs(void) {}

typedef struct
{
	char image[64];
	char title[64];
	char executable[64];
	rt_gfx_image_t* img;
}
app_info_t;

static app_info_t s_apps[32];
static int32_t s_apps_count = 0;
static const app_info_t* s_app_loading = 0;
static rt_gfx_image_t* s_banner = 0;
static int32_t s_anim_offset = 240;

static int ends_with(const char *str, const char *suffix)
{
    if (!str || !suffix)
        return 0;
    const size_t lenstr = strlen(str);
    const size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr)
        return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

static void read_no_lf(FILE* fp, char* buf)
{
	fgets(buf, 64, fp);
	char* ptr = strchr(buf, '\n');
	if (ptr) *ptr = 0;
}

static void enum_files(void* user, const char* filename, uint32_t size, uint8_t directory)
{
	if (ends_with(filename, ".nfo"))
	{
		FILE* fp = fopen(filename, "r");
		if (fp != 0)
		{
			read_no_lf(fp, s_apps[s_apps_count].image);
			read_no_lf(fp, s_apps[s_apps_count].title);
			read_no_lf(fp, s_apps[s_apps_count].executable);
			fclose(fp);

			s_apps[s_apps_count].img = rt_gfx_load_image(s_apps[s_apps_count].image);
			++s_apps_count;
		}
	}
}

static void load_available_applications()
{
	s_banner = rt_gfx_load_image("banner.pcx");
	file_enumerate("", 0, enum_files);
}

static void reset_available_applications()
{
	if (s_banner)
	{
		rt_gfx_destroy_image(s_banner);
		s_banner = 0;
	}
	for (int32_t i = 0; i < s_apps_count; ++i)
	{
		if (s_apps[i].img)
		{
			rt_gfx_destroy_image(s_apps[i].img);
			s_apps[i].img = 0;
		}
	}
	s_apps_count = 0;
	s_anim_offset = 240;
}

static const app_info_t* get_app_info_from_position(int32_t qx, int32_t qy)
{
	for (int32_t i = 0; i < 2; ++i)
	{
		for (int32_t j = 0; j < 3; ++j)
		{
			const int32_t x = j * 110 + 20;
			const int32_t y = (i + 1) * 110 + 20 + s_anim_offset;
			if (qx >= x && qy >= y && qx < x + 100 && qy < y + 100)
			{
				const int32_t idx = j + i * 3;
				if (idx < s_apps_count)
					return &s_apps[idx];
			}
		}
	}
	return 0;
}

void draw()
{
	const int32_t FW = 360;
	const int32_t FH = 360;

	rt_gfx_context_t cx;
	cx.width = 360;
	cx.height = 360;
	cx.pixels = rt_video_get_secondary_target();

	if (s_banner)
	{
		rt_video_blit(s_banner->pixels);
		rt_video_wait();
	}
	else
	{
		rt_video_clear(215);
		rt_video_wait();
	}

	if (!s_app_loading)
	{
		int32_t m[2];
		rt_input_get_absolute_position(m);

		for (int32_t i = 0; i < 2; ++i)
		{
			for (int32_t j = 0; j < 3; ++j)
			{
				const int32_t x = j * 110 + 20;
				const int32_t y = (i + 1) * 110 + 20 + s_anim_offset;
				const int32_t idx = j + i * 3;
				if (idx < s_apps_count && s_apps[idx].img != 0)
				{
					rt_gfx_blit_image_key(&cx, s_apps[idx].img, x, y);
					if (m[0] >= x && m[1] >= y && m[0] < x + 100 && m[1] < y + 100)
					{
						rt_gfx_draw_rect(&cx, x - 1, y - 1, 102, 102, 119);
						rt_gfx_draw_rect(&cx, x - 2, y - 2, 104, 104, 119);
					}
				}
				else
					rt_gfx_draw_rect(&cx, x, y, 100, 100, 162);
			}
		}
	}
	else
	{
		rt_gfx_draw_string(&cx, font8x8_c64, 20, 360 - 20, "Loading", 162);
		rt_gfx_draw_string(&cx, font8x8_c64, 20 + 8 * 8, 360 - 20, s_app_loading->title, 162);
	}

	rt_video_present(1);

	if (s_anim_offset > 0)
		s_anim_offset--;
}


void kickstart_main()
{
	// Initialize only systems which we need;
	// prevent linker from including unused code.
	rt_timer_init();
	hal_interrupt_init();
	rt_video_init();
	rt_kernel_init();

	rt_video_set_mode(VMODE_360_360_8);
	for (int32_t i = 0; i < 256; ++i)
	{
		const uint8_t* rgb = header_data_cmap[i];
		rt_video_set_palette(i, 
			(rgb[0] << 16) |
			(rgb[1] << 8) |
			rgb[2]);
	}
	rt_video_set_palette(1, 0x808080);
  	rt_video_set_palette(2, 0xffffff);
	rt_video_clear(215);
	rt_video_wait();
	rt_video_present(0);

	rt_input_init();

	draw();

	hal_uart_reset();
	rt_kernel_sleep(200);
	hal_uart_reset();
	rt_kernel_sleep(200);

	rt_input_show_cursor();

	int32_t card = 0;
	for (;;)
	{
		const int32_t chk = runtime_is_disk_connected();
		if (chk != card)
		{
			if (chk)
			{
				runtime_mount_disk();
				load_available_applications();
			}
			else
			{
				runtime_unmount_disk();
				reset_available_applications();
			}
			card = chk;
		}

		if (runtime_is_disk_mounted())
		{
			rt_event_t ev;
			while (rt_input_get_event(&ev))
			{
				if (ev.button == RT_INPUT_BUTTON_A || ev.button == RT_INPUT_TB)
				{
					const app_info_t* app = get_app_info_from_position(ev.x, ev.y);
					if (app)
					{
						s_app_loading = app;
						rt_input_hide_cursor();
						draw();

						rt_elf_launch(app->executable);

						s_app_loading = 0;
						rt_input_show_cursor();
						draw();
					}
				}
			}
		}

		// Check for commands on UART.
		if (!hal_uart_rx_empty())
		{
			remote_control();
		}

		draw();
	}
}

__attribute__((noreturn)) void error(const char* const msg)
{
	for (;;)
	{
		for (const char* ch = msg; *ch; ++ch)
			hal_uart_tx_u8(*ch);
	}
}

int main()
{
	// Initialize SP, since we hot restart and startup doesn't set SP.
	const uint32_t sp = 0x12000000 - 4;
	__asm__ volatile (
		"mv sp, %0	\n"
		:
		: "r" (sp)
	);
/*
	// Do some memory testing first.
	{
		volatile uint32_t* start = (volatile uint32_t*)0x10000000;
		volatile uint32_t* end = (volatile uint32_t*)0x12000000;
		
		for (volatile uint32_t* ptr = start; ptr != end; ++ptr)
		{
			*ptr = (uint32_t)ptr;
		}

		__asm__ volatile ("fence");

		for (volatile uint32_t* ptr = start; ptr != end; ++ptr)
		{
			const uint32_t value = *ptr;
			if (value != (uint32_t)ptr)
			{
				error("memory check 1 failed\n");
			}
			*ptr = ~(uint32_t)ptr;
		}

		__asm__ volatile ("fence");

		for (volatile uint32_t* ptr = start; ptr != end; ++ptr)
		{
			const uint32_t value = *ptr;
			if (value != ~(uint32_t)ptr)
			{
				error("memory check 2 failed\n");
			}
		}
	}
*/

	// Fill memory with EBREAK instructions.
	volatile uint32_t* start = (volatile uint32_t*)0x10000000;
	volatile uint32_t* end = (volatile uint32_t*)0x12000000;
	for (volatile uint32_t* ptr = start; ptr != end; ++ptr)
		*ptr = (uint32_t)0x00100073;
	__asm__ volatile ("fence");
	__asm__ volatile ("fence");

	// Initialize segments when running from ROM.
	{
		extern uint8_t INIT_DATA_VALUES;
		extern uint8_t INIT_DATA_START;
		extern uint8_t INIT_DATA_END;
		uint8_t* src = (uint8_t*)&INIT_DATA_VALUES;
		uint8_t* dest = (uint8_t*)&INIT_DATA_START;
		uint32_t len = (uint32_t)(&INIT_DATA_END - &INIT_DATA_START);
		memcpy(dest, src, len);
	}
	{
		extern uint8_t BSS_START;
		extern uint8_t BSS_END;
		uint8_t* dest = (uint8_t*)&BSS_START;
		uint32_t len = (uint32_t)(&BSS_END - &BSS_START);
		memset(dest, 0, len);
	}

	kickstart_main();
}
