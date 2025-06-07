/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

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

#include <HAL/Audio.h>
#include <HAL/I2C.h>
#include <HAL/SD.h>
#include <HAL/Timer.h>
#include <HAL/UART.h>

#include "Runtime/Audio.h"
#include "Runtime/CRT.h"
#include "Runtime/ELF.h"
#include "Runtime/File.h"
#include "Runtime/Input.h"
#include "Runtime/Runtime.h"

void __register_exitproc(void) {}
void __call_exitprocs(void) {}

int main()
{
	// Initialize SP, since we hot restart and startup doesn't set SP.
	const uint32_t sp = 0x22000000 - 4;
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
	
	if (input_init())
		printf("Failed to initialize input system.\n");

	if (rt_audio_init())
		printf("Failed to initialize audio system.\n");
	
	hal_sd_init(SD_MODE_HW);
	if (file_init())
		printf("Failed to initialize SD cards.\n");


	const int32_t fd = file_open("SONG", FILE_MODE_READ);
	if (fd < 0)
		printf("Failed to open file!\n");

	const int32_t fs = file_size(fd);
	printf("Reading music (%d bytes)...\n", fs);
	uint16_t* ptr = (uint16_t*)malloc(fs);
	
	uint8_t* dst = (uint8_t*)ptr;
	for (int32_t i = 0; i < fs; i += 512)
	{
		printf("... reading %d...\n", i);
		file_read(fd, dst, 512);
		dst += 512;
	}

	file_close(fd);

	printf("Playing music...\n");
	int32_t offset = 0;
	for (;;)
	{
		//input_update();

		printf("... play %d...\n", offset);

		rt_audio_play_mono(&ptr[offset], 1024);

		offset += 1024;
		if (offset * 2 >= fs)
			offset = 0;
	}

	/*

	1. Try to load "BOOT" elf from external SD card (if SD card inserted)
	2. If no external SD card then launch "DASH" from internal SD card.
	3. Dashboard just wait until external SD card inserted and then reset into firmware.

	** If external SD card with "NOBOOT" available then firmware should wait for
	** commands from serial port.

	*/

	printf("Launching dashboard...\n");
	const int32_t r = elf_launch("DASH");
	printf("Failed to launch, result = %d\n", r);
	for (;;);

	return 0;
}
