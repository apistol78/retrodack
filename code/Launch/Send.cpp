#include "Launch/Send.h"

#include <Core/Log/Log.h>
#include <Core/Misc/String.h>

using namespace traktor;

#define CW(s) { if (!(s)) return false; }

namespace
{

template < typename T >
bool write(traktor::IStream* target, T value)
{
	return target->write(&value, sizeof(T)) == sizeof(T);
}

template < typename T >
bool write(traktor::IStream* target, const T* value, int32_t count)
{
	const uint8_t* wp = (const uint8_t*)value;
	while (count > 0)
	{
		const int32_t nw = std::min< int32_t >(count, 256);
		const int32_t result = target->write(wp, nw * sizeof(T));
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
T read(traktor::IStream* target)
{
	T value = 0;
	target->read(&value, sizeof(T));
	return value;
}

template < typename T >
int32_t read(traktor::IStream* target, T* value, int32_t count)
{
	return (int32_t)target->read(value, count * sizeof(T));
}

}

bool sendLine(traktor::IStream* target, uint32_t base, const uint8_t* line, uint32_t length)
{
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

	return true;
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

	return true;
}
