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

typedef void (*call_fn_t)();

void __register_exitproc(void) {}
void __call_exitprocs(void) {}

static void remote_control()
{
	printf("[Firmware] Waiting on UART...\n");
	for (;;)
	{
		const uint8_t cmd = hal_uart_rx_u8();

		// "poke"
		if (cmd == 0x01)
		{
			const uint32_t addr = hal_uart_rx_u32();
			const uint16_t nb = hal_uart_rx_u16();
			uint8_t cs = 0;

			if (nb == 0 || nb > 1024)
			{
				hal_uart_tx_u8(0x81);	// Invalid data.
				continue;
			}

			// Add address to checksum.
			const uint8_t* p = (const uint8_t*)&addr;
			cs ^= p[0];
			cs ^= p[1];
			cs ^= p[2];
			cs ^= p[3];

			// Receive 
			uint8_t r[1024];
			for (uint16_t i = 0; i < nb; ++i)
			{
				const uint8_t d = hal_uart_rx_u8();
				r[i] = d;
				cs ^= d;
			}

			if (cs == hal_uart_rx_u8())
			{
				// Write data to memory.
				for (uint16_t i = 0; i < nb; ++i)
					*(volatile uint8_t*)(addr + i) = r[i];

				hal_uart_tx_u8(0x80);
			}
			else
				hal_uart_tx_u8(0x82);	// Invalid checksum.
		}

		// "peek"
		else if (cmd == 0x02)
		{
			const uint32_t addr = hal_uart_rx_u32();
			const uint16_t nb = hal_uart_rx_u16();

			if (nb == 0)
			{
				hal_uart_tx_u8(0x81);	// Invalid data.
				continue;
			}

			hal_uart_tx_u8(0x80);	// Ok

			for (uint16_t i = 0; i < nb; ++i)
				hal_uart_tx_u8(*(const volatile uint8_t*)(addr + i));
		}

		// "jump to"
		else if (cmd == 0x03)
		{
			const uint32_t addr = hal_uart_rx_u32();
			const uint32_t sp = hal_uart_rx_u32();
			uint8_t cs = 0;

			// Add address to checksum.
			{
				const uint8_t* p = (const uint8_t*)&addr;
				cs ^= p[0];
				cs ^= p[1];
				cs ^= p[2];
				cs ^= p[3];
			}

			// Add stack to checksum.
			{
				const uint8_t* p = (const uint8_t*)&sp;
				cs ^= p[0];
				cs ^= p[1];
				cs ^= p[2];
				cs ^= p[3];
			}

			if (cs == hal_uart_rx_u8())
			{
				hal_uart_tx_u8(0x80);	// Ok

				// Ensure DCACHE is flushed.
				__asm__ volatile ("fence");
				
				// Set initial stack pointer.
				if (sp != 0)
				{
					__asm__ volatile (
						"mv	sp, %0\n"
						:
						: "r" (sp)
					);
				}
				
				((call_fn_t)addr)();
			}
			else
				hal_uart_tx_u8(0x82);	// Invalid checksum.
		}

		// "echo"
		else
			hal_uart_tx_u8(cmd);
	}
}

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
	remote_control();

/*
	
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
*/

	/*

	1. Try to load "BOOT" elf from external SD card (if SD card inserted)
	2. If no external SD card then launch "DASH" from internal SD card.
	3. Dashboard just wait until external SD card inserted and then reset into firmware.

	** If external SD card with "NOBOOT" available then firmware should wait for
	** commands from serial port.

	*/

/*
	printf("Launching dashboard...\n");
	const int32_t r = elf_launch("DASH");
	printf("Failed to launch, result = %d\n", r);
	for (;;);
*/

	return 0;
}
