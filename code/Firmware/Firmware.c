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

#include <HAL/Interrupt.h>
#include <HAL/SD.h>
#include <HAL/Timer.h>
#include <HAL/UART.h>

#include "Runtime/Audio.h"
#include "Runtime/Console.h"
#include "Runtime/CRT.h"
#include "Runtime/ELF.h"
#include "Runtime/File.h"
#include "Runtime/Input.h"
#include "Runtime/Runtime.h"
#include "Runtime/Timer.h"
#include "Runtime/Kernel.h"
#include "Runtime/Video.h"

typedef void (*call_fn_t)();

void __register_exitproc(void) {}
void __call_exitprocs(void) {}

static uint8_t from_hex(char hex)
{
	if (hex >= '0' && hex <= '9')
		return hex - '0';
	else if (hex >= 'a' && hex <= 'f')
		return hex - 'a' + 10;
	else if (hex >= 'A' && hex <= 'F')
		return hex - 'A' + 10;
	else
		return 0;
}

static uint8_t rx_u8()
{
	const uint8_t h = from_hex(hal_uart_rx_u8());
	const uint8_t l = from_hex(hal_uart_rx_u8());
	return (h << 4) | l;
}

static uint16_t rx_u16()
{
	const uint8_t h = rx_u8();
	const uint8_t l = rx_u8();
	return (h << 8) | l;
}

static uint32_t rx_u32()
{
	const uint16_t h = rx_u16();
	const uint16_t l = rx_u16();
	return (h << 16) | l;
}

static void remote_control()
{
	uint8_t r[1024];
	// for (;;)
	{
		const uint8_t cmd = hal_uart_rx_u8();

		// "write"
		if (cmd == 'W')
		{
			const uint32_t addr = rx_u32();
			const uint16_t nb = rx_u16();
			uint8_t cs = 0;

			if (nb == 0 || nb > 1024)
			{
				hal_uart_tx_u8('E');
				// continue;
				return;
			}

			// Add address to checksum.
			const uint8_t* p = (const uint8_t*)&addr;
			cs ^= p[0];
			cs ^= p[1];
			cs ^= p[2];
			cs ^= p[3];

			// Receive 
			for (uint16_t i = 0; i < nb; ++i)
			{
				const uint8_t d = rx_u8();
				r[i] = d;
				cs ^= d;
			}

			if (cs == rx_u8())
			{
				// Write data to memory.
				for (uint16_t i = 0; i < nb; ++i)
					*(volatile uint8_t*)(addr + i) = r[i];

				hal_uart_tx_u8('O');
			}
			else
				hal_uart_tx_u8('E');	// Invalid checksum.
		}

		// "jump to"
		else if (cmd == 'J')
		{
			const uint32_t addr = rx_u32();
			const uint32_t sp = rx_u32();
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

			if (cs == rx_u8())
			{
				hal_uart_tx_u8('O');	// Ok

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
				hal_uart_tx_u8('E');	// Invalid checksum.
		}
	}
}

static void load(const char* game)
{
	if (hal_sd_init(SD_MODE_SW) == SD_RESULT_OK)
	{
		// Try to execute BOOT executable from SD
		// card, if available.
		rt_console_printf("Loading \"%s\"...\n", game);
		rt_kernel_sleep(200);

		file_init();
		rt_elf_launch(game);

		// No BOOT executable found.
		rt_console_printf("\"%s\" not found!\n", game);
	}
	else
	{
		// No SD card inserted.
		rt_console_printf("No SD card inserted.\n");
		rt_kernel_sleep(200);
	}
}

void kickstart_main()
{
	// Initialize only systems which we need;
	// prevent linker from including unused code.
	rt_timer_init();
	hal_interrupt_init();
	hal_video_init();
	rt_input_init();
	rt_kernel_init();
	rt_console_init();

	rt_console_printf("RetroDACK 0.1\n");
	rt_console_printf("\n");

	rt_console_printf("Press S1 for Doom\n");
	rt_console_printf("Press S2 for ScummRV\n");

	rt_kernel_sleep(200);

	for (;;)
	{
		rt_event_t ev;
		while (rt_input_get_event(&ev))
		{
			if (ev.button == RT_INPUT_BUTTON_S1)
				load("doom");
			if (ev.button == RT_INPUT_BUTTON_S2)
				load("scummrv");
		}
		
		rt_kernel_yield();

		while (!hal_uart_rx_empty())
			remote_control();
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
