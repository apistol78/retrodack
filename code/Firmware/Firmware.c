/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <HAL/Audio.h>
#include <HAL/I2C.h>
#include <HAL/SD.h>
#include <HAL/Timer.h>

#include "Runtime/CRT.h"
#include "Runtime/ELF.h"
#include "Runtime/File.h"
#include "Runtime/Runtime.h"

typedef void (*call_fn_t)();

void __register_exitproc(void) {}
void __call_exitprocs(void) {}

void main(int argc, const char** argv)
{
	// Initialize SP, since we hot restart and startup doesn't set SP.
	const uint32_t sp = 0x20000000 + 0x00800000;
	__asm__ volatile (
		"mv sp, %0	\n"
		:
		: "r" (sp)
	);

	// // Initialize segments when running from ROM.
	// {
	// 	extern uint8_t INIT_DATA_VALUES;
	// 	extern uint8_t INIT_DATA_START;
	// 	extern uint8_t INIT_DATA_END;
	// 	uint8_t* src = (uint8_t*)&INIT_DATA_VALUES;
	// 	uint8_t* dest = (uint8_t*)&INIT_DATA_START;
	// 	uint32_t len = (uint32_t)(&INIT_DATA_END - &INIT_DATA_START);
	// 	memcpy(dest, src, len);
	// }
	// {
	// 	extern uint8_t BSS_START;
	// 	extern uint8_t BSS_END;
    //     uint8_t* dest = (uint8_t*)&BSS_START;
    //     uint32_t len = (uint32_t)(&BSS_END - &BSS_START);
	// 	memset(dest, 0, len);
	// }

	crt_init();

	hal_sd_init(SD_INTERNAL_BASE, SD_MODE_SW);
	hal_sd_init(SD_EXTERNAL_BASE, SD_MODE_SW);

	file_init();

	elf_launch("0:/Dashboard");

	for (;;);
}
