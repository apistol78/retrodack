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
#include <HAL/Sprite.h>

#include "Runtime/Runtime.h"

#include "Runtime/USB/Max3420.h"
#include "Runtime/USB/UsbMassStorage.h"
#include "Runtime/USB/ScsiCommands.h"

#include "Firmware/Cursor.h"

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
				hal_uart_tx_u8('O');	// Ok
				rt_timer_wait_ms(100);	// Wait so UART have time to transmit response.

				// Disable interrupts; assumed to be reinitialized
				// by executable.
				hal_interrupt_disable();

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

static void load(const char* game)
{
	// Try to execute BOOT executable from SD
	// card, if available.
	rt_console_printf("Loading \"%s\"...\n", game);
	rt_kernel_sleep(200);

	rt_elf_launch(game);

	// No BOOT executable found.
	rt_console_printf("\"%s\" not found!\n", game);
}

void kickstart_main()
{
	// Initialize only systems which we need;
	// prevent linker from including unused code.
	rt_timer_init();
	hal_interrupt_init();
	rt_video_init();
	rt_kernel_init();
	rt_console_init();
	rt_input_init();

	max3420_init_device();

	rt_video_set_palette(2, 0xffffff);
	rt_video_set_palette(3, 0x000000);

	rt_console_printf("RetroDACK 0.2.1\n");
	rt_console_printf("\n");

	hal_uart_reset();
	rt_kernel_sleep(200);
	hal_uart_reset();
	rt_kernel_sleep(200);

	hal_sprite_set_visible(0, 0xff);
	hal_sprite_set_bits(0, c_mouseCursor, 32, 32);
	rt_input_set_hotspot(16, 16);

	int32_t card = SD_RESULT_NO_CARD;

	for (;;)
	{
		const int32_t chk = hal_sd_card_inserted();
		if (chk != card)
		{
			if (chk == SD_RESULT_OK)
			{
				rt_console_printf("Initialize SD...\n");
				hal_sd_init(SD_MODE_SW);
				
				rt_console_printf("Initialize DISK...\n");
				rt_disk_init();
				
				rt_console_printf("Initialize FILE...\n");
				file_init();

				rt_console_printf("Initialize USB...\n");
				max3420_init_usb();
				// scsi_write_enable(1);

				rt_console_printf("\n");
				rt_console_printf("Press S1 for Doom\n");
				rt_console_printf("Press S2 for ScummRV\n");
				rt_console_printf("Press  A for Quake\n");
				rt_console_printf("\n");
			}
			else
			{
				rt_console_printf("Terminate USB...\n");
				max3420_terminate_usb();
			}
			card = chk;
		}

		if (card == SD_RESULT_OK)
		{
			// Process user input.
			rt_event_t ev;
			while (rt_input_get_event(&ev))
			{
				if (ev.button == RT_INPUT_BUTTON_S1)
				{
					max3420_usb_suspend();
					load("doom");
				}
				else if (ev.button == RT_INPUT_BUTTON_S2)
				{
					max3420_usb_suspend();
					load("scummrv");
				}
				else if (ev.button == RT_INPUT_BUTTON_A)
				{
					max3420_usb_suspend();
					load("quake");
				}
			}

			// Process USB event.
			const usbEvent_t event = max3420_get_usb_event();
			switch (event)
			{
				case USB_VBUS_LOST:
					break;
					
				case BUS_RESET:
					usb_mass_process_bus_reset();
					break;
					
				case SETUP_PACKET_AVAILABLE:
					usb_mass_process_setup_packet();
					break;
					
				case EP1_OUT_DATA:
					usb_mass_process_bulk_out_transaction();
					break;
					
				case USB_SUSPEND:
					max3420_usb_suspend();
					break;
					
				default:
					break;
			}			
		}
		else
		{
			rt_kernel_sleep(100);
		}

		// Check for commands on UART.
		if (!hal_uart_rx_empty())
		{
			rt_console_printf("\nEntering remote control...\n");

			if (card == SD_RESULT_OK)
				max3420_terminate_usb();

			remote_control();
		}

		// // Check battery.
		// battery_t bat;
		// rt_battery_read(&bat);
		// rt_console_printf("voltage     : %d mV\n", bat.voltage);
		// rt_console_printf("f avail cap : %d mAh\n", bat.fullAvailableCapacity);
		// rt_console_printf("f charge cap: %d mAh\n", bat.fullChargeCapacity);
		// rt_console_printf("r capacity  : %d mAh\n", bat.remainingCapacity);
		// rt_console_printf("current     : %d mA\n", bat.current);
		// rt_console_printf("power       : %d mW\n", bat.power);
		// rt_console_printf("state       : %d %%\n", bat.stageOfCharge);
		// rt_console_printf("\n");	
	}
}

__attribute__((noreturn)) void error(const char* const msg)
{
	for (;;)
	{
		for (const char* ch = msg; *ch; ++ch)
			hal_uart_tx_u8(*ch);
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
/*
	// Do some memory testing first.
	{
		volatile uint32_t* start = (volatile uint32_t*)0x10000000;
		volatile uint32_t* end = (volatile uint32_t*)0x12000000;
		
		for (volatile uint32_t* ptr = start; ptr != end; ++ptr)
		{
			*ptr = (uint32_t)ptr;
		}

		__asm__ volatile ("fence");

		for (volatile uint32_t* ptr = start; ptr != end; ++ptr)
		{
			const uint32_t value = *ptr;
			if (value != (uint32_t)ptr)
			{
				error("memory check 1 failed\n");
			}
			*ptr = ~(uint32_t)ptr;
		}

		__asm__ volatile ("fence");

		for (volatile uint32_t* ptr = start; ptr != end; ++ptr)
		{
			const uint32_t value = *ptr;
			if (value != ~(uint32_t)ptr)
			{
				error("memory check 2 failed\n");
			}
		}
	}
*/
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
