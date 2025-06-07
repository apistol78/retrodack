/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include <string.h>

#include <Core/Containers/StaticVector.h>
#include <Core/Io/AnsiEncoding.h>
#include <Core/Io/BufferedStream.h>
#include <Core/Io/DynamicMemoryStream.h>
#include <Core/Io/FileSystem.h>
#include <Core/Io/FileOutputStream.h>
#include <Core/Io/StreamCopy.h>
#include <Core/Io/StringReader.h>
#include <Core/Io/Utf8Encoding.h>
#include <Core/Log/Log.h>
#include <Core/Log/LogRedirectTarget.h>
#include <Core/Math/Random.h>
#include <Core/Misc/CommandLine.h>
#include <Core/Misc/String.h>
#include <Core/Thread/ThreadManager.h>
#include <Core/Thread/Thread.h>

#include "Launch/ELF.h"
#include "Launch/Send.h"
#include "Launch/Serial.h"

using namespace traktor;

bool uploadBinary(traktor::IStream* target, const std::wstring& fileName, uint32_t offset)
{
	Ref< traktor::IStream > f = FileSystem::getInstance().open(fileName, File::FmRead);
	if (!f)
	{
		log::error << L"Unable to open binary \"" << fileName << L"\"." << Endl;
		return false;
	}

	uint32_t linear = offset;
	uint8_t data[16];
	for (;;)
	{
		const int64_t r = f->read(data, sizeof(data));
		if (r <= 0)
			break;

		log::info << L"DATA " << str(L"%08x", linear) << L"..." << Endl;

		if (!sendLine(target, linear, data, 16))
			return false;

		linear += 16;
	}

	log::info << L"Binary uploaded successfully." << Endl;
	return true;
}

bool uploadELF(traktor::IStream* target, const std::wstring& fileName, uint32_t sp)
{
	AlignedVector< uint8_t > elf;
	uint32_t start = -1;
	uint32_t last = 0;

	// Read entire ELF into memory.
	{
		Ref< traktor::IStream > f = FileSystem::getInstance().open(fileName, File::FmRead);
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

			log::info << L"PT_LOAD " << str(L"0x%08x", addr) << L" - file size " << str(L"0x%08x", addr + phdr[i].p_filesz)  << L" - mem size " << str(L"0x%08x", addr + phdr[i].p_memsz) << Endl;

			for (uint32_t j = 0; j < phdr[i].p_filesz; j += 1024)
			{
				const uint32_t cnt = std::min< uint32_t >(phdr[i].p_filesz - j, 1024);
				log::info << L"TEXT " << str(L"%08x", addr + j) << L" (" << cnt << L" bytes)..." << Endl;
				if (!sendLine(target, addr + j, pbits + j, cnt))
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
				{
					start = sym[j].st_value;
					break;
				}
			}
		}
	}

	if (start != -1)
	{
		if (sp)
			log::info << L"JUMP " << str(L"0x%08x", start) << L", SP " << str(L"0x%08x", sp) << Endl;
		else
			log::info << L"JUMP " << str(L"0x%08x", start) << Endl;

		if (!sendJump(target, start, sp))
			return false;
	}

	return true;
}

bool uploadHEX(traktor::IStream* target, const std::wstring& fileName, uint32_t sp)
{
	StaticVector< uint8_t, 16 > record;
	std::wstring tmp;

	Ref< traktor::IStream > ff = FileSystem::getInstance().open(fileName, File::FmRead);
	if (!ff)
	{
		log::error << L"Unable to open HEX \"" << fileName << L"\"." << Endl;
		return false;
	}

	Ref< traktor::IStream > f = new BufferedStream(ff);

	int64_t fileSize = f->available();

	uint32_t segment = 0x00000000;
	uint32_t upper = 0x00000000;
	uint32_t start = 0xffffffff;
	uint32_t end = 0x00000000;

	StringReader sr(f, new AnsiEncoding());
	while (sr.readLine(tmp) >= 0)
	{
		if (tmp.empty())
			continue;

		// Ensure start byte is correct.
		if (tmp[0] != L':')
			continue;
		tmp = tmp.substr(1);

		const int32_t percent = ((f->tell() * 100) / fileSize);

		// Parse header.
		const int32_t ln = parseString< int32_t >(L"0x" + tmp.substr(0, 2));
		const int32_t type = parseString< int32_t >(L"0x" + tmp.substr(6, 2));
		int32_t addr = parseString< int32_t >(L"0x" + tmp.substr(2, 4));

		if (type == 0x00)
		{
			if (ln > 16)
			{
				log::error << L"Too long record." << Endl;
				return false;
			}

			log::info << L"TEXT " << str(L"%08x", (upper | addr) + segment) << L" (" << percent << L"%)..." << Endl;

			const uint32_t linear = (upper | addr) + segment;

			// Parse record.
			record.resize(0);
			for (int32_t i = 8; i < 8 + ln * 2; i += 2)
			{
				const int32_t v = parseString< int32_t >(L"0x" + tmp.substr(i, 2));
				record.push_back((uint8_t)v);
			}

			if (!sendLine(target, linear, record.c_ptr(), record.size()))
				return false;

			addr += record.size();

			start = std::min< uint32_t >(start, linear);
			end = std::max< uint32_t >(end, linear);
		}
		else if (type == 0x01)
			break;
		else if (type == 0x02)
		{
			segment = parseString< int32_t >(L"0x" + tmp.substr(8, 4));
			segment *= 16;
		}
		else if (type == 0x03)
			continue;
		else if (type == 0x04)
		{
			upper = parseString< int32_t >(L"0x" + tmp.substr(8, 4));
			upper <<= 16;
		}
		else if (type == 0x05)
		{
			const uint32_t linear = parseString< uint32_t >(L"0x" + tmp.substr(8, 8));
			
			if (sp != 0)
				log::info << L"JUMP " << str(L"%08x", linear) << L" (SP: 0x" << str(L"%08x", sp) << L")..." << Endl;
			else
				log::info << L"JUMP " << str(L"%08x", linear) << L"..." << Endl;

			if (!sendJump(target, linear, sp))
				return false;

			// Cannot load more since target is executing.
			break;
		}
		else
			log::warning << L"Unhandled HEX record type " << type << L"." << Endl;
	}

	log::info << L"HEX uploaded, adress range " << str(L"0x%08x", start) << L" - " << str(L"0x%08x", end) << L"." << Endl;
	return true;
}

int main(int argc, const char** argv)
{
	CommandLine commandLine(argc, argv);

	std::wstring device = L"/dev/ttyACM0";
	if (commandLine.hasOption('d', L"device"))
		device = commandLine.getOption('d', L"device").getInteger();

	Serial::Configuration configuration;
	configuration.baudRate = 115200;
	configuration.stopBits = 1;
	configuration.parity = Serial::Parity::No;
	configuration.byteSize = 8;
	configuration.dtrControl = Serial::DtrControl::Disable;

	Ref< Serial > serial = new Serial();
	if (!serial->open(device, configuration))
	{
		log::error << L"Unable to open serial device " << device << L"." << Endl;
		return 1;
	}

	Ref< traktor::IStream > target = serial;

	// Issue reset command.
	// if (commandLine.hasOption(L"reset"))
	// {
	// 	write< uint8_t >(target, 0xff);
	// 	ThreadManager::getInstance().getCurrentThread()->sleep(200);
	// }

	// Purge incoming data.
	for (;;)
	{
		ThreadManager::getInstance().getCurrentThread()->sleep(200);
		if (target->available() == 0)
			break;
		while (target->available() > 0)
		{
			uint8_t dummy;
			if (target->read(&dummy, 1) <= 0)
			{
				log::error << L"Serial device error while purging." << Endl;
				return 1;
			}
		}
	}

	uint32_t sp = 0x22000000 - 4;
	if (commandLine.hasOption('s', L"stack"))
		sp = (uint32_t)commandLine.getOption('s', L"stack").getInteger();

	if (commandLine.hasOption('e', L"elf"))
	{
		const std::wstring elf = commandLine.getOption('e', L"elf").getString();
		if (!uploadELF(target, elf, sp))
		{
			log::error << L"Unable to load ELF." << Endl;
			return 1;
		}
	}

	// if (commandLine.hasOption('r', L"raw"))
	// {
	// 	const std::wstring file = commandLine.getOption('r', L"raw").getString();
	// 	if (!uploadBinary(target, file))
	// 	{
	// 		log::error << L"Unable to upload file." << Endl;
	// 		return 1;
	// 	}
	// }

	for (;;)
	{
		if (target->available() > 0)
		{
			uint8_t ch;
			if (target->read(&ch, 1) <= 0)
				break;
			if (!iscntrl(ch))
				log::info << wchar_t(ch);
			else if (ch == '\n')
				log::info << Endl;
		}
		else
			ThreadManager::getInstance().getCurrentThread()->sleep(100);
	}

	return 0;
}