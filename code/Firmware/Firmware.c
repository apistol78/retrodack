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
#include <HAL/UART.h>

#include "Runtime/CRT.h"
#include "Runtime/ELF.h"
#include "Runtime/File.h"
#include "Runtime/Runtime.h"

void __register_exitproc(void) {}
void __call_exitprocs(void) {}

const volatile uint8_t check[] = { 0x11, 0x22, 0x33, 0x44 };

int main()
{
	// Initialize SP, since we hot restart and startup doesn't set SP.
	const uint32_t sp = 0x21E00000 - 4;
	__asm__ volatile (
		"mv sp, %0	\n"
		:
		: "r" (sp)
	);

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

	crt_init();

	// printf("checking memory...\n");
	// for (uint32_t i = 0x20000000; i < 0x21E00000; i += 4)
	// {
	// 	*((volatile uint32_t*)i) = i;
	// }
	// for (uint32_t i = 0x20000000; i < 0x21E00000; i += 4)
	// {
	// 	const uint32_t v = *((volatile uint32_t*)i);
	// 	if (v != i)
	// 	{
	// 		printf("memory fail at %08x (%08x)...\n", i, v);
	// 		for (;;);
	// 	}
	// }


	for (uint64_t a = 0; a <= 100; a += 10)
	{
		for (uint64_t b = 0; b <= 100; b += 10)
		{
			uint64_t r = a * b;

			printf("%08x:%08x\n", (uint32_t)(a >> 32), (uint32_t)a);
			printf("%08x:%08x\n", (uint32_t)(b >> 32), (uint32_t)b);
			printf("%08x:%08x\n", (uint32_t)(r >> 32), (uint32_t)r);
			// printf("%08x:%08x\n", b >> 32, b);

			//printf("%lu * %lu = %lu\n", a, b, r);
		}
	}

/*
	hal_sd_init(SD_MODE_HW);
	if (file_init())
		printf("file_init failed\n");

	printf("Launcing dashboard...\n");
	int32_t r = elf_launch("DASH");
	printf("Failed to launch, result = %d\n", r);
*/
	for (;;);

	return 0;
}
