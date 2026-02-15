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

// #include "Runtime/USB/Max3420.h"
// #include "Runtime/USB/UsbMassStorage.h"
// #include "Runtime/USB/ScsiCommands.h"

typedef void __attribute__((noreturn)) (*call_fn_t)();

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

static void load(const char* game)
{
	// Try to execute BOOT executable from SD
	// card, if available.
	// rt_console_printf("Loading \"%s\"...\n", game);
	rt_kernel_sleep(200);

	rt_elf_launch(game);

	// No BOOT executable found.
	// rt_console_printf("\"%s\" not found!\n", game);
}



static int ends_with(const char *str, const char *suffix)
{
    if (!str || !suffix)
        return 0;
    const size_t lenstr = strlen(str);
    const size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr)
        return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}


typedef struct
{
	char image[64];
	char executable[64];
	rt_gfx_image_t* img;
}
app_info_t;

static app_info_t s_apps[32];
static int32_t s_apps_count = 0;


static void enum_files(void* user, const char* filename, uint32_t size, uint8_t directory)
{
	if (ends_with(filename, ".nfo"))
	{
		FILE* fp = fopen(filename, "r");
		if (fp != 0)
		{
			fgets(s_apps[s_apps_count].image, 64, fp);
			fgets(s_apps[s_apps_count].executable, 64, fp);
			fclose(fp);

			printf("loading thumb \"%s\"...\n", s_apps[s_apps_count].image);

			s_apps[s_apps_count].img = rt_gfx_load_image(s_apps[s_apps_count].image);

			++s_apps_count;
		}
	}
}


static void load_available_applications()
{
	printf("scanning applications...\n");
	file_enumerate("", 0, enum_files);
}


void draw()
{
	rt_gfx_context_t cx;
	cx.width = 360;
	cx.height = 360;
	cx.pixels = rt_video_get_secondary_target();

	int32_t m[2];
	rt_input_get_absolute_position(m);

	for (int32_t i = 0; i < 3; ++i)
	{
		for (int32_t j = 0; j < 3; ++j)
		{
			int32_t x = j * 108 + 18;
			int32_t y = i * 108 + 18;

			if (m[0] >= x && m[1] >= y && m[0] < x + 100 && m[1] < y + 100)
			{
				x -= 4;
				y -= 4;
			}

			int32_t idx = j + i * 3;
			if (idx < s_apps_count && s_apps[idx].img != 0)
			{
				rt_gfx_blit_image(&cx, s_apps[idx].img, x, y);
			}
			else
				rt_gfx_fill_rect(&cx, x, y, 100, 100, 1);
		}
	}

	rt_video_present(1);
}


void kickstart_main()
{
	// Initialize only systems which we need;
	// prevent linker from including unused code.
	rt_timer_init();
	hal_interrupt_init();
	rt_video_init();
	rt_kernel_init();
	// rt_console_init();
	rt_input_init();

	// max3420_init_device();

	// rt_console_printf("RetroDACK 0.2.4\n");
	// rt_console_printf("\n");

	rt_video_set_mode(VMODE_360_360_8);
	rt_video_set_palette(0, 0x808080);
  	rt_video_set_palette(1, 0xff0000);

	hal_uart_reset();
	rt_kernel_sleep(200);
	hal_uart_reset();
	rt_kernel_sleep(200);

	int32_t card = SD_RESULT_NO_CARD;

	rt_event_t ev;
	while (rt_input_get_event(&ev));

	for (;;)
	{
		const int32_t chk = hal_sd_card_inserted();
		if (chk != card)
		{
			if (chk == SD_RESULT_OK)
			{
				// rt_console_printf("Initialize SD...\n");
				hal_sd_init(SD_MODE_SW);
				
				// rt_console_printf("Initialize DISK...\n");
				rt_disk_init();
				
				// rt_console_printf("Initialize FILE...\n");
				file_init();

				// rt_console_printf("Initialize USB...\n");
				// max3420_init_usb();
				// scsi_write_enable(1);

				// rt_console_printf("\n");
				// rt_console_printf("Press S1 for Doom\n");
				// rt_console_printf("Press S2 for ScummRV\n");
				// rt_console_printf("Press  A for Tetris\n");
				// rt_console_printf("\n");

				load_available_applications();
			}
			// else
			// {
			// 	rt_console_printf("Terminate USB...\n");
			// 	max3420_terminate_usb();
			// }
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
					// max3420_usb_suspend();
					load("doom");
				}
				else if (ev.button == RT_INPUT_BUTTON_S2)
				{
					// max3420_usb_suspend();
					load("scummrv");
				}
				else if (ev.button == RT_INPUT_BUTTON_A)
				{
					// max3420_usb_suspend();
					load("tetris");
				}
			}

			// // Process USB event.
			// const usbEvent_t event = max3420_get_usb_event();
			// switch (event)
			// {
			// 	case USB_VBUS_LOST:
			// 		break;
					
			// 	case BUS_RESET:
			// 		usb_mass_process_bus_reset();
			// 		break;
					
			// 	case SETUP_PACKET_AVAILABLE:
			// 		usb_mass_process_setup_packet();
			// 		break;
					
			// 	case EP1_OUT_DATA:
			// 		usb_mass_process_bulk_out_transaction();
			// 		break;
					
			// 	case USB_SUSPEND:
			// 		max3420_usb_suspend();
			// 		break;
					
			// 	default:
			// 		break;
			// }			
		}
		else
		{
			rt_kernel_sleep(100);
		}

		// Check for commands on UART.
		if (!hal_uart_rx_empty())
		{
			// rt_console_printf("\nEntering remote control...\n");

			// if (card == SD_RESULT_OK)
			// 	max3420_terminate_usb();

			remote_control();
		}

		draw();
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

	// Fill memory with EBREAK instructions.
	volatile uint32_t* start = (volatile uint32_t*)0x10000000;
	volatile uint32_t* end = (volatile uint32_t*)0x12000000;
	for (volatile uint32_t* ptr = start; ptr != end; ++ptr)
		*ptr = (uint32_t)0x00100073;
	__asm__ volatile ("fence");
	__asm__ volatile ("fence");

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
