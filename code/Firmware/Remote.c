/*
 RetroDÄCK
 Copyright (c) 2026 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include "Runtime/Runtime.h"

#include <HAL/Interrupt.h>
#include <HAL/UART.h>

#include "Firmware/Remote.h"

typedef void __attribute__((noreturn)) (*call_fn_t)();

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

void remote_control()
{
	static char filename[256];
	static int32_t fd;
	static uint8_t r[1024];

	for (;;)
	{
		// wait until any data has been receieved.
		while (hal_uart_rx_empty())
			rt_kernel_yield();

		const uint8_t cmd = hal_uart_rx_u8();
		if (cmd == 'W')	// "write"
		{
			const uint32_t addr = rx_u32();
			const uint16_t nb = rx_u16();
			uint8_t cs = 0;

			if (nb == 0 || nb > 1024)
			{
				hal_uart_tx_u8('E');
				continue;
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

				// Flush DCACHE.
				__asm__ volatile ("fence");

				// Verify data in memory.
				uint8_t result = 'O';
				for (uint16_t i = 0; i < nb; ++i)
				{
					if (*(volatile uint8_t*)(addr + i) != r[i])
					{
						result = 'E';
						break;
					}
				}

				hal_uart_tx_u8(result);
			}
			else
				hal_uart_tx_u8('E');	// Invalid checksum.
		}
		else if (cmd == 'J')	// "jump to"
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
				// Disable interrupts; assumed to be reinitialized
				// by executable.
				hal_interrupt_disable();

				hal_uart_tx_u8('O');	// Ok
				rt_timer_wait_ms(250);	// Wait so UART have time to transmit response.

				// Ensure DCACHE is flushed.
				__asm__ volatile (
					"fence	\n"
					"fence	\n"
				);

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
		else
		{
			hal_uart_tx_u8('E');	// Unknown command.
		}
	}
}
