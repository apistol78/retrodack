/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "Firmware/ELF.h"
#include "Runtime/CRT.h"
#include "Runtime/File.h"
#include "Runtime/Runtime.h"
#include "Runtime/HAL/I2C.h"
#include "Runtime/HAL/Interrupt.h"
#include "Runtime/HAL/SD.h"
#include "Runtime/HAL/SystemRegisters.h"
#include "Runtime/HAL/Timer.h"
#include "Runtime/HAL/UART.h"

typedef void (*call_fn_t)();

void __register_exitproc(void) {}
void __call_exitprocs(void) {}

static int32_t launch_elf(const char* filename)
{
	printf("open \"%s\"...\n", filename);

	int32_t fd = file_open(filename, FILE_MODE_READ);
	if (fd <= 0)
	{
		printf("unable to open \"%s\"\n", filename);
		return 1;
	}

	char tmp[256] = {};
	uint32_t jstart = 0;

	printf("read header ...\n");

	ELF32_Header hdr = {};
	file_read(fd, (uint8_t*)&hdr, sizeof(hdr));
	if (hdr.e_machine != 0xf3)
	{
		printf("invalid ELF header\n");
		return 2;
	}

	printf("reading %d sections...\n", hdr.e_shnum);
	for (uint32_t i = 0; i < hdr.e_shnum; ++i)
	{
		ELF32_SectionHeader shdr = {};
		file_seek(fd, hdr.e_shoff + i * sizeof(ELF32_SectionHeader), 0);
		file_read(fd, (uint8_t*)&shdr, sizeof(shdr));

		if (
			shdr.sh_type == 0x01 ||	// SHT_PROGBITS
			shdr.sh_type == 0x0e ||	// SHT_INIT_ARRAY
			shdr.sh_type == 0x0f	// SHT_FINI_ARRAY
		)
		{
			if ((shdr.sh_flags & 0x02) == 0x02)	// SHF_ALLOC
			{
				printf("reading section at 0x%08x (%d bytes)...\n", shdr.sh_addr, shdr.sh_size);
				file_seek(fd, shdr.sh_offset, 0);
				for (uint32_t i = 0; i < shdr.sh_size; i += 512)
				{
					uint32_t nb = shdr.sh_size - i;
					if (nb > 512)
						nb = 512;
					if (file_read(fd, (void*)(shdr.sh_addr + i), nb) != nb)
						return 3;
				}
			}
		}
		else if (shdr.sh_type == 0x02)	// SHT_SYMTAB
		{
			ELF32_SectionHeader shdr_link;
			file_seek(fd, hdr.e_shoff + shdr.sh_link * sizeof(ELF32_SectionHeader), 0);
			file_read(fd, (uint8_t*)&shdr_link, sizeof(shdr_link));

			for (int32_t j = 0; j < shdr.sh_size; j += sizeof(ELF32_Sym))
			{
				ELF32_Sym sym = {};
				file_seek(fd, shdr.sh_offset + j, 0);
				file_read(fd, (uint8_t*)&sym, sizeof(sym));

				file_seek(fd, shdr_link.sh_offset + sym.st_name, 0);
				file_read(fd, tmp, sym.st_size);

				tmp[sym.st_size] = 0;

				if (strcmp(tmp, "_start") == 0)
				{
					jstart = sym.st_value;
					break;
				}
			}
		}
	}

	file_close(fd);

	if (jstart != 0)
	{
		const uint32_t sp = 0x20000000 + sysreg_read(SR_REG_RAM_SIZE) - 0x8;
		printf("launching application (stack @ 0x%08x)...\n", sp);
		__asm__ volatile (
			"fence					\n"
			"mv		sp, %0			\n"
			:
			: "r" (sp)
		);
		((call_fn_t)jstart)();
	}
	
	printf("no start address\n");
	return 4;
}

static void remote_control()
{
	printf("waiting on UART...\n");
	for (;;)
	{
		uint8_t cmd = uart_rx_u8();
		sysreg_write(SR_REG_LEDS, cmd);

		// poke
		if (cmd == 0x01)
		{
			const uint32_t addr = uart_rx_u32();
			const uint16_t nb = uart_rx_u16();
			uint8_t cs = 0;

			if (nb == 0 || nb > 1024)
			{
				uart_tx_u8(0x81);	// Invalid data.
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
				uint8_t d = uart_rx_u8();
				r[i] = d;
				cs ^= d;
			}

			if (cs == uart_rx_u8(0))
			{
				// Write data to memory.
				for (uint16_t i = 0; i < nb; ++i)
					*(uint8_t*)(addr + i) = r[i];

				uart_tx_u8(0x80);
			}
			else
				uart_tx_u8(0x82);	// Invalid checksum.
		}

		// peek
		else if (cmd == 0x02)
		{
			const uint32_t addr = uart_rx_u32(0);
			const uint16_t nb = uart_rx_u16(0);

			if (nb == 0)
			{
				uart_tx_u8(0x81);	// Invalid data.
				continue;
			}

			uart_tx_u8(0x80);	// Ok

			for (uint16_t i = 0; i < nb; ++i)
				uart_tx_u8(*(uint8_t*)(addr + i));
		}

		// jump to
		else if (cmd == 0x03)
		{
			const uint32_t addr = uart_rx_u32();
			const uint32_t sp = uart_rx_u32();
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

			if (cs == uart_rx_u8())
			{
				uart_tx_u8(0x80);	// Ok

				// Ensure DCACHE is flushed.
				__asm__ volatile ("fence");
				
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
				uart_tx_u8(0x82);	// Invalid checksum.
		}

		// echo
		else
			uart_tx_u8(cmd);
	}
}

#define TRACKBALL_REG_LED_RED 0x00
#define TRACKBALL_REG_LED_GRN 0x01
#define TRACKBALL_REG_LED_BLU 0x02
#define TRACKBALL_REG_LED_WHT 0x03

#define TRACKBALL_REG_LEFT 0x04
#define TRACKBALL_REG_RIGHT 0x05
#define TRACKBALL_REG_UP 0x06
#define TRACKBALL_REG_DOWN 0x07
#define TRACKBALL_REG_SWITCH 0x08
#define TRACKBALL_MSK_SWITCH_STATE 0b10000000

#define TRACKBALL_REG_USER_FLASH 0xD0
#define TRACKBALL_REG_FLASH_PAGE 0xF0
#define TRACKBALL_REG_INT 0xF9
#define TRACKBALL_MSK_INT_TRIGGERED 0b00000001
#define TRACKBALL_MSK_INT_OUT_EN 0b00000010
#define TRACKBALL_REG_CHIP_ID_L 0xFA
#define TRACKBALL_RED_CHIP_ID_H 0xFB
#define TRACKBALL_REG_VERSION 0xFC
#define TRACKBALL_REG_I2C_ADDR 0xFD
#define TRACKBALL_REG_CTRL 0xFE
#define TRACKBALL_MSK_CTRL_SLEEP 0b00000001
#define TRACKBALL_MSK_CTRL_RESET 0b00000010
#define TRACKBALL_MSK_CTRL_FREAD 0b00000100
#define TRACKBALL_MSK_CTRL_FWRITE 0b00001000


volatile int32_t g_i2c_counter = 0;
volatile int32_t x = 0, y = 0;

void i2c_handler(uint32_t source)
{
	++g_i2c_counter;

	uint8_t data[5];
	i2c_read(0x0a, TRACKBALL_REG_LEFT, data, 5);
	
	if (data[0])
		x += data[0];
	if (data[1])
		x -= data[1];
	if (data[2])
		y += data[2];
	if (data[3])
		y -= data[3];

}

void main(int argc, const char** argv)
{
	// Initialize SP, since we hot restart and startup doesn't set SP.
	const uint32_t sp = 0x20000000 + sysreg_read(SR_REG_RAM_SIZE);
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

	int32_t i = 0;
	
	for (;;)
	{
		printf("request chip id...\n\r");

		uint8_t data[5];
		i2c_read(0x0a, TRACKBALL_REG_CHIP_ID_L, data, 2);

		if (data[0] == 0x11 && data[1] == 0xba)
		{
			printf("found trackball!\n\r");

			i2c_write(0x0a, TRACKBALL_REG_LED_GRN, 0xff);

			i2c_read(0x0a, TRACKBALL_REG_INT, data, 1);
			data[0] |= TRACKBALL_MSK_INT_OUT_EN;
			i2c_write(0x0a, TRACKBALL_REG_INT, data[0]);

			interrupt_init();
			interrupt_set_handler(IRQ_SOURCE_PLIC_0, i2c_handler);
			interrupt_enable();

			for (;;)
			{
				printf("(%d) %d : %d\n\r", g_i2c_counter, x, y);
				timer_wait_ms(100);
			}
		}
		else
		{
			printf("not trackball found!\n\r");
		}

		timer_wait_ms(1000);
	}

	for (;;);

	/*
	printf("initialize storage...\n");
	sd_init(SD_MODE_SW);

	printf("initialize file system...\n");
	file_init();

	if ((sysreg_read(SR_REG_FLAGS) & 0x01) == 0x00)
		launch_elf("BOOT");
	else
		remote_control();

	for (;;);
	*/
}
