#include <Core/Log/Log.h>
#include <Core/Misc/String.h>

#include "Emulator/RTC.h"

using namespace traktor;

T_IMPLEMENT_RTTI_CLASS(L"RTC", RTC, I2C::ISlave)

void RTC::write(uint8_t controlAddr, uint8_t data)
{
}

void RTC::read(uint8_t controlAddr, uint8_t length, uint8_t* outData)
{
	switch (controlAddr)
	{
	// Second
	case 0x00:
		*outData = 0x00;
		break;

	// Minute
	case 0x01:
		*outData = 0x00;
		break;

	// Hour
	case 0x02:
		*outData = 0x00;
		break;

	// Month day
	case 0x03:
		*outData = 0x00;
		break;

	// Month
	case 0x04:
		*outData = 0x00;
		break;

	// Year
	case 0x05:
		*outData = 26;
		break;
	}
}

bool RTC::tick(ICPU* cpu)
{
	return true;
}
