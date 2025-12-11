#include "Launch/Send.h"

#include <Core/Log/Log.h>
#include <Core/Misc/String.h>
#include <Core/Thread/Thread.h>
#include <Core/Thread/ThreadManager.h>

using namespace traktor;

#define CW(s) { if (!(s)) return false; }

namespace
{

bool writeChar(traktor::IStream* target, uint8_t v)
{
	return target->write(&v, 1) == 1;
}

uint8_t readChar(traktor::IStream* target)
{
	uint8_t ch = 0;
	target->read(&ch, 1);
	return ch;
}

bool writeU8(traktor::IStream* target, uint8_t v)
{
	const char hex[] = "0123456789abcdef";
	const uint8_t h = (v >> 4);
	const uint8_t l = v & 15;
	if (target->write(&hex[h], 1) != 1)
		return false;
	if (target->write(&hex[l], 1) != 1)
		return false;
	return true;
}

bool writeU16(traktor::IStream* target, uint16_t v)
{
	const uint8_t h = (v >> 8);
	const uint8_t l = v & 255;
	if (!writeU8(target, h))
		return false;
	if (!writeU8(target, l))
		return false;
	return true;
}

bool writeU32(traktor::IStream* target, uint32_t v)
{
	const uint16_t h = (v >> 16);
	const uint16_t l = v & 65535;
	if (!writeU16(target, h))
		return false;
	if (!writeU16(target, l))
		return false;
	return true;
}

}

bool sendWrite(traktor::IStream* target, uint32_t base, const uint8_t* line, uint32_t length)
{
	uint8_t cs = 0;
	uint8_t reply;

	// Add address to checksum.
	const uint8_t* p = (const uint8_t*)&base;
	cs ^= p[0];
	cs ^= p[1];
	cs ^= p[2];
	cs ^= p[3];

	// Parse record and calculate checksum.
	for (uint32_t i = 0; i < length; ++i)
		cs ^= line[i];

	for (int32_t tr = 0; tr < 10; ++tr)
	{
		CW(writeChar(target, 'W'));
		CW(writeU32(target, base));
		CW(writeU16(target, (uint16_t)length));
		for (uint32_t i = 0; i < length; ++i)
			CW(writeU8(target, line[i]));
		CW(writeU8(target, cs));

		reply = readChar(target);
		if (reply == 'O')
			return true;

		log::warning << L"Error reply, trying again..." << Endl;

		// Purge incoming data.
		for (;;)
		{
			ThreadManager::getInstance().getCurrentThread()->sleep(20);
			if (target->available() == 0)
				break;
			while (target->available() > 0)
			{
				uint8_t ch;
				if (target->read(&ch, 1) <= 0)
				{
					log::error << L"Serial device error while purging." << Endl;
					return false;
				}
			}
		}
	}

	log::error << L"Error reply, got " << str(L"0x%02x", reply) << Endl;
	return false;
}

bool sendJump(traktor::IStream* target, uint32_t start, uint32_t sp)
{
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

	CW(writeChar(target, 'J'));
	CW(writeU32(target, start));
	CW(writeU32(target, sp));
	CW(writeU8(target, cs));

	const uint8_t reply = readChar(target);
	if (reply != 'O')
	{
		log::error << L"Error reply, got " << str(L"0x%02x", reply) << Endl;
		return false;
	}

	return true;
}

bool sendCreateFile(traktor::IStream* target, const char* fileName)
{
	CW(writeChar(target, 'f'));
	for (const char* ch = fileName; *ch; ++ch)
	{
		CW(writeChar(target, *ch));
	}
	CW(writeChar(target, 0));

	const uint8_t reply = readChar(target);
	if (reply != 'O')
	{
		log::error << L"Error reply, got " << str(L"0x%02x", reply) << Endl;
		return false;
	}

	return true;	
}

bool sendWriteFile(traktor::IStream* target, const uint8_t* line, uint32_t length)
{
	uint8_t reply;

	if (length <= 0)
		return true;

	for (int32_t tr = 0; tr < 10; ++tr)
	{	
		CW(writeChar(target, 'w'));
		CW(writeU16(target, (uint16_t)length));
		for (uint32_t i = 0; i < length; ++i)
			CW(writeU8(target, line[i]));

		reply = readChar(target);
		if (reply == 'O')
			return true;

		log::warning << L"Error reply, trying again..." << Endl;

		// Purge incoming data.
		for (;;)
		{
			ThreadManager::getInstance().getCurrentThread()->sleep(1000);
			if (target->available() == 0)
				break;
			while (target->available() > 0)
			{
				uint8_t ch;
				if (target->read(&ch, 1) <= 0)
				{
					log::error << L"Serial device error while purging." << Endl;
					return false;
				}
			}
		}
	}

	log::error << L"Error reply, got " << str(L"0x%02x", reply) << Endl;
	return false;	
}

bool sendCloseFile(traktor::IStream* target)
{
	CW(writeChar(target, 'c'));

	const uint8_t reply = readChar(target);
	if (reply != 'O')
	{
		log::error << L"Error reply, got " << str(L"0x%02x", reply) << Endl;
		return false;
	}

	return true;	
}
