/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include <stdio.h>

#include <HAL/Interrupt.h>
#include <HAL/SD.h>
#include <HAL/Timer.h>
#include <HAL/UART.h>
#include <HAL/Video.h>

#include "Runtime/Audio.h"
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

	printf("** Initialize IRQ handler **\n");
	hal_interrupt_init();
	
	printf("** Initialize Video **\n");
	if (hal_video_init() != 0)
		printf("Video init failed!\n");

	printf("** Initialize Audio **\n");
	rt_audio_init();

	printf("** Initialize SD card **\n");
	if (hal_sd_init(SD_MODE_SW) != 0)	// HW is only available on HW (duh), not emulator...
		printf("SD card init failed!\n");

	printf("** Initialize FS **\n");
	if (file_init() != 0)
		printf("FS init failed!\n");

	printf("** Initialize Input **\n");
	input_init();

	printf("** Initialize Kernel **\n");
	kernel_init();

	printf("** Ready **\n");
    return 0;
}

void runtime_warm_restart()
{
	typedef void (*call_fn_t)();

	const uint32_t sp = 0x22000000 - 4;
	__asm__ volatile (
		"mv sp, %0	\n"
		:
		: "r" (sp)

	);

	const uint32_t addr = 0x00000000;
	((call_fn_t)addr)();

	for (;;);
}
