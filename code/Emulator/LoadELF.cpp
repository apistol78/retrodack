#include <cstring>

#include <Core/Io/DynamicMemoryStream.h>
#include <Core/Io/FileSystem.h>
#include <Core/Io/IStream.h>
#include <Core/Io/StreamCopy.h>
#include <Core/Log/Log.h>
#include <Core/Misc/String.h>
#include <Core/Misc/TString.h>

#include <Emulator2/CPU/Bus.h>
#include <Emulator2/CPU/ICPU.h>
#include <Emulator2/CPU/HL/BusAccess.h>
#include <Emulator2/CPU/HL/DCache.h>

#include "Emulator/LoadELF.h"

using namespace traktor;

#define EI_NIDENT 16

#pragma pack(1)
struct ELF32_Header
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
};
#pragma pack()

#pragma pack(1)
struct ELF32_ProgramHeader
{
	uint32_t p_type;
	uint32_t p_offset;
	uint32_t p_vaddr;
	uint32_t p_paddr;
	uint32_t p_filesz;
	uint32_t p_memsz;
	uint32_t p_flags;
	uint32_t p_align;
};
#pragma pack()

#pragma pack(1)
struct ELF32_SectionHeader
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
};
#pragma pack()

#pragma pack(1)
struct ELF32_Sym
{
	uint32_t st_name;
	uint32_t st_value;
	uint32_t st_size;
	uint8_t st_info;
	uint8_t st_other;
	uint16_t st_shndx;
};
#pragma pack()

bool loadELF(const std::wstring& fileName, ICPU& cpu, Bus& bus)
{
	AlignedVector< uint8_t > elf;

	// Temporary dcache & bus access so we can write bytes.
	DCache dcache(&bus);
	BusAccess busAccess(&dcache);

	// Read entire ELF into memory.
	{
		Ref< IStream > f = FileSystem::getInstance().open(fileName, File::FmRead);
		if (!f)
		{
			log::error << L"Unable to open ELF \"" << fileName << L"\"." << Endl;
			return false;
		}

		DynamicMemoryStream dms(elf, false, true);
		if (!StreamCopy(&dms, f).execute())
		{
			log::error << L"Unable to open ELF \"" << fileName << L"\"; failed to read file." << Endl;
			return false;
		}
	}

	auto hdr = (const ELF32_Header*)elf.c_ptr();
	if (hdr->e_machine != 0xF3)
	{
		log::error << L"Unable to parse ELF \"" << fileName << L"\"; incorrect machine type." << Endl;
		return false;		
	}

	auto phdr = (const ELF32_ProgramHeader*)(elf.c_ptr() + hdr->e_phoff);
	for (uint32_t i = 0; i < hdr->e_phnum; ++i)
	{
		if (phdr[i].p_type == 0x01) // PT_LOAD
		{
			const auto pbits = (const uint8_t*)(elf.c_ptr() + phdr[i].p_offset);
			const uint32_t addr = phdr[i].p_paddr;
			for (uint32_t j = 0; j < phdr[i].p_filesz; ++j)
			{
				busAccess.writeU8(0, addr + j, pbits[j]);
				if (bus.error())
					return false;
			}
		}
	}

	auto shdr = (const ELF32_SectionHeader*)(elf.c_ptr() + hdr->e_shoff);
	for (uint32_t i = 0; i < hdr->e_shnum; ++i)
	{
		if (shdr[i].sh_type == 0x02)	// SHT_SYMTAB
		{
			const char* strings = (const char*)(elf.c_ptr() + shdr[shdr[i].sh_link].sh_offset);
			auto sym = (const ELF32_Sym*)(elf.c_ptr() + shdr[i].sh_offset);
			for (int32_t j = 0; j < shdr[i].sh_size / sizeof(ELF32_Sym); ++j)
			{
				const char* name = strings + sym[j].st_name;
				if (strcmp(name, "_start") == 0)
					cpu.jump(sym[j].st_value);
			}
		}
	}

	dcache.flush();
	return true;
}
