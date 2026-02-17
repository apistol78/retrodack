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

	printf("** Initialize Disk **\n");
	rt_disk_init();

	printf("** Initialize SD card **\n");
	const int32_t result = hal_sd_init(SD_MODE_SW);
	if (result != SD_RESULT_OK)
		printf("SD card init failed (result = %d)!\n", result);

	printf("** Initialize FS **\n");
	if (file_init() != 0)
		printf("FS init failed!\n");

	printf("** Initialize Input **\n");
	rt_input_init();

	printf("** Ready **\n");
    return 0;
}

void runtime_warm_restart()
{
	typedef void (*call_fn_t)();

	hal_interrupt_disable();
	hal_sprite_set_visible(0, 0x00);

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
