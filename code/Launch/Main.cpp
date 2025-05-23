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

#include <Net/Network.h>
#include <Net/SocketAddressIPv4.h>
#include <Net/SocketStream.h>
#include <Net/UdpSocket.h>

#include "Launch/ELF.h"
#include "Launch/Serial.h"

using namespace traktor;

template < typename T >
bool write(IStream* target, T value)
{
	return target->write(&value, sizeof(T)) == sizeof(T);
}

template < typename T >
bool write(IStream* target, const T* value, int32_t count)
{
	const uint8_t* wp = (const uint8_t*)value;
	while (count > 0)
	{
		const int32_t nw = std::min< int32_t >(count, 256);
		int32_t result = target->write(wp, nw * sizeof(T));
		if (result > 0)
		{
			wp += result;
			count -= result;
		}
		else
			return false;
	}
	return true;
}

template < typename T >
T read(IStream* target)
{
	T value = 0;
	target->read(&value, sizeof(T));
	return value;
}

template < typename T >
void read(IStream* target, T* value, int32_t count)
{
	target->read(value, count * sizeof(T));
}

bool sendLine(IStream* target, uint32_t base, const uint8_t* line, uint32_t length)
{
#define CW(s) { if (!(s)) return false; }
	uint8_t cs = 0;

	// Add address to checksum.
	const uint8_t* p = (const uint8_t*)&base;
	cs ^= p[0];
	cs ^= p[1];
	cs ^= p[2];
	cs ^= p[3];

	// Parse record and calculate checksum.
	for (uint32_t i = 0; i < length; ++i)
		cs ^= line[i];

	CW(write< uint8_t >(target, 0x01));
	CW(write< uint32_t >(target, base));
	CW(write< uint16_t >(target, (uint16_t)length));
	CW(write< uint8_t >(target, line, length));
	CW(write< uint8_t >(target, cs));

	const uint8_t reply = read< uint8_t >(target);
	if (reply != 0x80)
	{
		log::error << L"Error reply, got " << str(L"%02x", reply) << Endl;
		return false;
	}

#undef CW
	return true;
}

bool sendJump(IStream* target, uint32_t start, uint32_t sp)
{
#define CW(s) { if (!(s)) return false; }
	uint8_t cs = 0;

	// Add address to checksum.
	{
		const uint8_t* p = (const uint8_t*)&start;
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

	CW(write< uint8_t >(target, 0x03));
	CW(write< uint32_t >(target, start));
	CW(write< uint32_t >(target, sp));
	CW(write< uint8_t >(target, cs));

	const uint8_t reply = read< uint8_t >(target);
	if (reply != 0x80)
	{
		log::error << L"Error reply, got " << str(L"%02x", reply) << Endl;
		return false;
	}

#undef CW
	return true;
}

bool uploadImage(IStream* target, const std::wstring& fileName, uint32_t offset)
{
	Ref< traktor::IStream > f = FileSystem::getInstance().open(fileName, File::FmRead);
	if (!f)
	{
		log::error << L"Unable to open image \"" << fileName << L"\"." << Endl;
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

	log::info << L"Image uploaded" << Endl;
	return true;
}

bool uploadELF(IStream* target, const std::wstring& fileName, uint32_t sp)
{
	AlignedVector< uint8_t > elf;
	uint32_t start = -1;
	uint32_t last = 0;

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

	auto shdr = (const ELF32_SectionHeader*)(elf.c_ptr() + hdr->e_shoff);
	for (uint32_t i = 0; i < hdr->e_shnum; ++i)
	{
		if (
			shdr[i].sh_type == 0x01 ||	// SHT_PROGBITS
			shdr[i].sh_type == 0x0e ||	// SHT_INIT_ARRAY
			shdr[i].sh_type == 0x0f		// SHT_FINI_ARRAY
		)
		{
			if ((shdr[i].sh_flags & 0x02) == 0x02)	// SHF_ALLOC
			{
				const auto pbits = (const uint8_t*)(elf.c_ptr() + shdr[i].sh_offset);
				const uint32_t addr = shdr[i].sh_addr;

				for (uint32_t j = 0; j < shdr[i].sh_size; j += 1024)
				{
					const uint32_t cnt = std::min< uint32_t >(shdr[i].sh_size - j, 1024);
					log::info << L"TEXT " << str(L"%08x", addr + j) << L" (" << cnt << L" bytes)..." << Endl;
					if (!sendLine(target, addr + j, pbits + j, cnt))
						return false;
				}

				last = std::max(last, addr + shdr[i].sh_size);
			}
		}
		else if (shdr[i].sh_type == 0x02)	// SHT_SYMTAB
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

bool uploadHEX(IStream* target, const std::wstring& fileName, uint32_t sp)
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

bool uploadFile(IStream* target, const std::wstring& fileName)
{
	while (target->available() > 0)
	{
		uint8_t dummy;
		target->read(&dummy, 1);
	}

	Ref< traktor::IStream > ff = FileSystem::getInstance().open(fileName, File::FmRead);
	if (!ff)
	{
		log::error << L"Unable to open file \"" << fileName << L"\"." << Endl;
		return false;
	}

	Ref< traktor::IStream > f = new BufferedStream(ff);

	const int32_t avail = f->available();

	uint8_t buf[1024];
	int32_t total = 0;

	for (;;)
	{
		int32_t nr = std::min(avail - total, 1024);
		if (nr <= 0)
			break;

		if (f->read(buf, nr) != nr)
			break;

		log::info << str(L"%08x -> %08x", total, (total + nr) - 1) << Endl;

		// Calculate checksum.
		uint8_t cs = 0;
		for (int32_t i = 0; i < nr; ++i)
			cs ^= buf[i];		

		write< uint32_t >(target, (uint32_t)nr);
		write< uint8_t >(target, buf, nr);
		write< uint8_t >(target, cs);

		const uint8_t reply = read< uint8_t >(target);
		if (reply == 0x81)
		{
			log::warning << L"Detected corrupt transmission; resending..." << Endl;

			write< uint32_t >(target, (uint32_t)nr);
			write< uint8_t >(target, buf, nr);
			write< uint8_t >(target, cs);

			const uint8_t replyAgain = read< uint8_t >(target);
			if (replyAgain != 0x80)
			{
				log::error << L"Resend failed; aborting." << Endl;
				return false;
			}
		}
		else if (reply != 0x80)
		{
			log::error << L"Error reply, got " << str(L"%02x", reply) << Endl;
			write< uint8_t >(target, (uint8_t)0x00);
			return false;
		}

		total += nr;
	}

	write< uint32_t >(target, 0);

	log::info << L"File uploaded." << Endl;
	return true;
}

void for_each(uint32_t from, uint32_t to, uint32_t step, const std::function< void(uint32_t) >& fn)
{
	if (from <= to)
	{
		for (uint32_t addr = from; addr <= to; addr += step)
			fn(addr);
	}
	else
	{
		for (uint32_t addr = from; addr >= to; addr -= step)
			fn(addr);
	}
}

bool memcheck(IStream* target, uint32_t from, uint32_t to, uint32_t step)
{
	uint8_t utd[64];
	uint8_t ind[64];

	uint8_t cnt = 0;
	uint32_t error = 0;

	const uint32_t nb = 64;
	const uint32_t seed = clock();

	Random rnd = Random(seed);
	for_each(from, to, step, [&](uint32_t addr)
	{
		for (int32_t i = 0; i < nb; ++i)
			utd[i] = (uint8_t)rnd.next();

		log::info << L"S " << str(L"%08x", addr) << L": ";
		for (int32_t i = 0; i < nb; ++i)
			log::info << str(L"%02x", utd[i]) << L" ";
		log::info << Endl;

		// Write random data.
		uint8_t cs = 0;

		const uint8_t* p = (const uint8_t*)&addr;
		cs ^= p[0];
		cs ^= p[1];
		cs ^= p[2];
		cs ^= p[3];

		for (int32_t i = 0; i < nb; ++i)
			cs ^= (uint8_t)utd[i];

		write< uint8_t >(target, 0x01);
		write< uint32_t >(target, addr);
		write< uint16_t >(target, nb);
		write< uint8_t >(target, utd, nb);
		write< uint8_t >(target, cs);

		uint8_t reply = read< uint8_t >(target);
		if (reply != 0x80)
		{
			log::error << L"Error reply (write), got " << str(L"%02x", reply) << Endl;
			return;
		}
	});

	rnd = Random(seed);
	for_each(from, to, step, [&](uint32_t addr)
	{
		for (int32_t i = 0; i < nb; ++i)
			utd[i] = (uint8_t)rnd.next();

		// Read back data.
		write< uint8_t >(target, 0x02);
		write< uint32_t >(target, addr);
		write< uint16_t >(target, nb);

		uint8_t reply = read< uint8_t >(target);
		if (reply != 0x80)
		{
			log::error << L"Error reply (read), got " << str(L"%02x", reply) << Endl;
			return;
		}
		read< uint8_t >(target, ind, nb);

		log::info << L"R " << str(L"%08x", addr) << L": ";
		for (int32_t i = 0; i < nb; ++i)
			log::info << str(L"%02x", ind[i]) << L" ";

		if (memcmp(ind, utd, nb) != 0)
		{
			log::info << L"MISMATCH!";
			error++;
		}

		log::info << Endl;
	});

	if (error > 0)
		log::error << error << L" error(s) found." << Endl;
	else
		log::info << L"No errors found." << Endl;

	return error == 0;
}

uint8_t echoRnd()
{
	uint8_t v = 0;
	do { v = rand() & 255; } while(v == 0x01 || v == 0x02 || v == 0x03);
	return v;
}

int main(int argc, const char** argv)
{
	CommandLine commandLine(argc, argv);

	Ref< Serial > serial;
	Ref< net::UdpSocket > socket;
	Ref< IStream > target;

	if (!commandLine.hasOption(L"udp"))
	{
		int32_t port = 0;
		if (commandLine.hasOption('p', L"port"))
			port = commandLine.getOption('p', L"port").getInteger();

		Serial::Configuration configuration;
		configuration.baudRate = 460800;
		configuration.stopBits = 1;
		configuration.parity = Serial::Parity::No;
		configuration.byteSize = 8;
		configuration.dtrControl = Serial::DtrControl::Disable;

		serial = new Serial();
		if (!serial->open(port, configuration))
		{
			log::error << L"Unable to open serial port " << port << L"." << Endl;
			return 1;
		}

		target = serial;
	}
	else
	{
		net::Network::initialize();

		socket = new net::UdpSocket();
		if (!socket->bind(net::SocketAddressIPv4(45123)))
		{
			log::error << L"Unable to bind socket to port." << Endl;
			return 1;
		}

		target = new net::SocketStream(socket, true, true, 1000);
	}

	// Issue reset command.
	if (commandLine.hasOption(L"reset"))
	{
		write< uint8_t >(target, 0xff);
		ThreadManager::getInstance().getCurrentThread()->sleep(200);
	}

	// Purge incoming data.
	for (;;)
	{
		ThreadManager::getInstance().getCurrentThread()->sleep(200);
		if (target->available() == 0)
			break;
		while (target->available() > 0)
			read< uint8_t >(target);
	}

	if (commandLine.hasOption(L"memcheck"))
	{
		const uint32_t from = commandLine.getOption(L"memcheck-from").getInteger();
		const uint32_t to = commandLine.getOption(L"memcheck-to").getInteger();
		
		uint32_t step = 64;
		if (commandLine.hasOption(L"memcheck-step"))
			step = commandLine.getOption(L"memcheck-step").getInteger();

		memcheck(target, from, to, step);
		return 0;
	}

	uint32_t sp = 0;
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

	if (commandLine.hasOption('r', L"raw"))
	{
		const std::wstring file = commandLine.getOption('r', L"raw").getString();
		if (!uploadFile(target, file))
		{
			log::error << L"Unable to upload file." << Endl;
			return 1;
		}
	}

	for (;;)
	{
		if (target->available() > 0)
		{
			const uint8_t ch = read< uint8_t >(target);
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