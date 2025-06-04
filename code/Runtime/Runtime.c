/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include <stdio.h>

#include <HAL/Audio.h>
#include <HAL/Interrupt.h>
#include <HAL/SD.h>
#include <HAL/Timer.h>
#include <HAL/UART.h>
#include <HAL/Video.h>

#include "Runtime/CRT.h"
#include "Runtime/File.h"
#include "Runtime/Input.h"
#include "Runtime/Kernel.h"
#include "Runtime/Runtime.h"

// Needed by custom printf implementation.
void _putchar(char character)
{
	hal_uart_tx_u8(character);
}

int32_t runtime_init()
{
	crt_init();

	// printf("** Initialize IRQ handler **\n");
	// hal_interrupt_init();
	
	// printf("** Initialize Video **\n");
	// if (hal_video_init() != 0)
	// 	printf("Video init failed!\n");

	// printf("** Initialize Audio **\n");
	// hal_audio_init();

	// printf("** Initialize SD card **\n");
	// if (hal_sd_init(SD_MODE_SW) != 0)
	// 	printf("SD (internal) init failed!\n");

	// printf("** Initialize FS **\n");
	// if (file_init() != 0)
	// 	printf("FS init failed!\n");

	// printf("** Initialize Input **\n");
	// input_init();

	// printf("** Initialize Kernel **\n");
	// kernel_init();

	printf("** Ready **\n");
    return 0;
}

void runtime_warm_restart()
{
	typedef void (*call_fn_t)();

	hal_video_set_palette(0, 0x00000000);
	hal_video_clear(0);
	hal_video_present(0);

	const uint32_t sp = 0x20100000 + /*sysreg_read(SR_REG_RAM_SIZE)*/ 0x01000000;
	__asm__ volatile (
		"mv sp, %0	\n"
		:
		: "r" (sp)

	);

	const uint32_t addr = 0x00000000;
	((call_fn_t)addr)();

	for (;;);
}

void runtime_cold_restart()
{
	hal_video_set_palette(0, 0x00000000);
	hal_video_clear(0);
	hal_video_present(0);

	// sysreg_modify(SR_REG_FLAGS, 0b100, 0b100);

	for (;;);
}
