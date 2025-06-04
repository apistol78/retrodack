/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include <stdio.h>
#include <string.h>

#include "Runtime/ELF.h"
#include "Runtime/File.h"

typedef void (*call_fn_t)();

int32_t elf_launch(const char* filename)
{
	uint8_t tmp[8000];
	ELF32_Header hdr;
	ELF32_ProgramHeader phdr;
	ELF32_SectionHeader shdr;
	ELF32_SectionHeader shdr_link;
	ELF32_Sym sym;
	uint32_t jstart = 0;

	const int32_t fd = file_open(filename, FILE_MODE_READ);
	if (fd <= 0)
		return 1;

	printf("%s opened as %d...\n", filename, fd);

	// Read header and ensure it's the correct machine type.
	memset(&hdr, 0, sizeof(hdr));
	file_read(fd, (uint8_t*)&hdr, sizeof(hdr));
	if (hdr.e_machine != 0xf3)
		return 2;

	for (uint32_t i = 0; i < hdr.e_phnum; ++i)
	{
		printf("reading program header %u...\n", i);

		file_seek(fd, hdr.e_phoff + i * sizeof(ELF32_ProgramHeader), 0);
		file_read(fd, (uint8_t*)&phdr, sizeof(phdr));

		printf("... p_type = 0x%02x\n", phdr.p_type);

		if (phdr.p_type == 0x01) // PT_LOAD
		{
			printf("PT_LOAD, offset %u, filesz %u, memsz %u, paddr 0x%08x, vaddr 0x%08x\n", phdr.p_offset, phdr.p_filesz, phdr.p_memsz, phdr.p_paddr, phdr.p_vaddr);

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
		printf("reading section header %u...\n", i);

		file_seek(fd, hdr.e_shoff + i * sizeof(ELF32_SectionHeader), 0);
		file_read(fd, (uint8_t*)&shdr, sizeof(shdr));

		/*if (
			shdr.sh_type == 0x01 ||	// SHT_PROGBITS
			shdr.sh_type == 0x0e ||	// SHT_INIT_ARRAY
			shdr.sh_type == 0x0f	// SHT_FINI_ARRAY
		)
		{
			if ((shdr.sh_flags & 0x02) == 0x02)	// SHF_ALLOC
			{
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
		else*/ if (shdr.sh_type == 0x02)	// SHT_SYMTAB
		{
			file_seek(fd, hdr.e_shoff + shdr.sh_link * sizeof(ELF32_SectionHeader), 0);
			file_read(fd, (uint8_t*)&shdr_link, sizeof(shdr_link));

			for (int32_t j = 0; j < shdr.sh_size; j += sizeof(ELF32_Sym))
			{
				file_seek(fd, shdr.sh_offset + j, 0);
				file_read(fd, (uint8_t*)&sym, sizeof(sym));

				if (sym.st_size >= sizeof(tmp))
					return 4;

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
		printf("jumping to 0x%08x...\n", jstart);

		const uint32_t sp = 0x22000000 - 4;
		__asm__ volatile (
			"fence					\n"
			"mv		sp, %0			\n"
			:
			: "r" (sp)
		);
		((call_fn_t)jstart)();
	}

	return 5;
}
