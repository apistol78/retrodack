/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include <stdio.h>
#include <string.h>

#include <HAL/Interrupt.h>

#include "Runtime/ELF.h"
#include "Runtime/File.h"
#include "Runtime/Video.h"

#define EI_NIDENT 16

#pragma pack(1)
typedef struct
{
	uint8_t e_ident[EI_NIDENT];
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint32_t e_entry;
	uint32_t e_phoff;
	uint32_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
} ELF32_Header;
#pragma pack()

#pragma pack(1)
typedef struct
{
	uint32_t p_type;
	uint32_t p_offset;
	uint32_t p_vaddr;
	uint32_t p_paddr;
	uint32_t p_filesz;
	uint32_t p_memsz;
	uint32_t p_flags;
	uint32_t p_align;
} ELF32_ProgramHeader;
#pragma pack()

#pragma pack(1)
typedef struct
{
	uint32_t sh_name;
	uint32_t sh_type;
	uint32_t sh_flags;
	uint32_t sh_addr;
	uint32_t sh_offset;
	uint32_t sh_size;
	uint32_t sh_link;
	uint32_t sh_info;
	uint32_t sh_addralign;
	uint32_t sh_entsize;
} ELF32_SectionHeader;
#pragma pack()

#pragma pack(1)
typedef struct
{
	uint32_t st_name;
	uint32_t st_value;
	uint32_t st_size;
	uint8_t st_info;
	uint8_t st_other;
	uint16_t st_shndx;
} ELF32_Sym;
#pragma pack()

typedef void (*call_fn_t)();

int32_t rt_elf_launch(const char* filename)
{
	uint8_t tmp[256];
	ELF32_Header hdr;
	ELF32_ProgramHeader phdr;
	ELF32_SectionHeader shdr;
	ELF32_SectionHeader shdr_link;
	ELF32_Sym sym;
	uint32_t jstart = 0;

	const int32_t fd = file_open(filename, FILE_MODE_READ);
	if (fd <= 0)
		return 1;

	// Read header and ensure it's the correct machine type.
	memset(&hdr, 0, sizeof(hdr));
	file_read(fd, &hdr, sizeof(hdr));
	if (hdr.e_machine != 0xf3)
		return 2;

	for (uint32_t i = 0; i < hdr.e_phnum; ++i)
	{
		file_seek(fd, hdr.e_phoff + i * sizeof(ELF32_ProgramHeader), 0);
		file_read(fd, &phdr, sizeof(phdr));

		if (phdr.p_type == 0x01) // PT_LOAD
		{
			file_seek(fd, phdr.p_offset, 0);
			for (uint32_t i = 0; i < phdr.p_filesz; i += 512)
			{
				uint32_t nb = phdr.p_filesz - i;
				if (nb > 512)
					nb = 512;
				if (file_read(fd, (void*)(phdr.p_paddr + i), nb) != nb)
					return 3;
			}
		}
	}

	for (uint32_t i = 0; i < hdr.e_shnum; ++i)
	{
		file_seek(fd, hdr.e_shoff + i * sizeof(ELF32_SectionHeader), 0);
		file_read(fd, (uint8_t*)&shdr, sizeof(shdr));
		if (shdr.sh_type == 0x02)	// SHT_SYMTAB
		{
			file_seek(fd, hdr.e_shoff + shdr.sh_link * sizeof(ELF32_SectionHeader), 0);
			file_read(fd, &shdr_link, sizeof(shdr_link));

			for (int32_t j = 0; j < shdr.sh_size; j += sizeof(ELF32_Sym))
			{
				file_seek(fd, shdr.sh_offset + j, 0);
				file_read(fd, &sym, sizeof(sym));

				if (sym.st_size >= sizeof(tmp))
					continue;

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
		rt_video_set_palette(0, 0x000000);
		rt_video_clear(0);
		rt_video_wait();
		rt_video_present();

		// Disable interrupts; assumed to be reinitialized
		// by executable.
		hal_interrupt_disable();

		const uint32_t sp = 0x12000000 - 4;
		__asm__ volatile (
			"fence			\n"
			"fence			\n"
			"mv		sp, %0	\n"
			:
			: "r" (sp)
		);
		((call_fn_t)jstart)();
	}

	return 5;
}
