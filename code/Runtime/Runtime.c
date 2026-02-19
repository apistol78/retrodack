/*
 RetroDÄCK
 Copyright (c) 2025-2026 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include <stdio.h>

#include <HAL/Interrupt.h>
#include <HAL/SD.h>
#include <HAL/Sprite.h>
#include <HAL/UART.h>

#include "Runtime/Runtime.h"

static int32_t s_disk_mounted = 0;

// Needed by custom printf implementation.
void _putchar(char character)
{
	hal_uart_tx_u8(character);
}

int32_t runtime_init()
{
	printf("** Initialize Timer **\n");
	rt_timer_init();

	printf("** Initialize IRQ handler **\n");
	hal_interrupt_init();

	printf("** Initialize I2C **\n");
	rt_i2c_init();

	printf("** Initialize Kernel **\n");
	rt_kernel_init();

	printf("** Initialize Video **\n");
	rt_video_init();

	printf("** Initialize Audio **\n");
	rt_audio_init();

	printf("** Initialize Input **\n");
	rt_input_init();

	// Automatically mount SD if card is inserted into reader.
	if (runtime_is_disk_connected())
		runtime_mount_disk();

	printf("** Ready **\n");
    return 0;
}

int32_t runtime_is_disk_connected()
{
	const int32_t chk = hal_sd_card_inserted();
	return (chk == SD_RESULT_OK) ? 1 : 0;
}

int32_t runtime_mount_disk()
{
	if (s_disk_mounted)
		return 0;

	const int32_t result = hal_sd_init(SD_MODE_SW);
	if (result != SD_RESULT_OK)
		return 1;

	if (file_init() != 0)
	{
		hal_sd_shutdown();
		return 1;
	}

	s_disk_mounted = 1;
	return 0;
}

void runtime_unmount_disk()
{
	if (!s_disk_mounted)
		return;

	hal_sd_shutdown();
	file_shutdown();
	s_disk_mounted = 0;
}

int32_t runtime_is_disk_mounted()
{
	return s_disk_mounted;
}

void runtime_warm_restart()
{
	typedef void (*call_fn_t)();

	hal_interrupt_disable();
	hal_sprite_set_visible(0, 0x00);
	hal_sd_shutdown();

	const uint32_t sp = 0x12000000 - 4;
	__asm__ volatile (
		"mv sp, %0	\n"
		:
		: "r" (sp)
	);

	const uint32_t addr = 0x00000000;
	((call_fn_t)addr)();

	for (;;);
}
