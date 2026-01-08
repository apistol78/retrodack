#include <Core/Log/Log.h>
#include <Core/Misc/String.h>

#include "Emulator/FuelGauge.h"

using namespace traktor;

T_IMPLEMENT_RTTI_CLASS(L"FuelGauge", FuelGauge, I2C::ISlave)

void FuelGauge::write(uint8_t controlAddr, uint8_t data)
{
}

void FuelGauge::read(uint8_t controlAddr, uint8_t length, uint8_t* outData)
{
	switch (controlAddr)
	{
	// Voltage
	case 0x04:
		T_ASSERT(length == 2);
		*(uint16_t*)outData = 0x0000;
		break;

	// Full available capacity.
	case 0x0a:
		T_ASSERT(length == 2);
		*(uint16_t*)outData = 0x0000;
		break;

	// Full charge capacity.
	case 0x0e:
		T_ASSERT(length == 2);
		*(uint16_t*)outData = 0x0000;
		break;

	// Remaining capacity.
	case 0x0c:
		T_ASSERT(length == 2);
		*(uint16_t*)outData = 0x0000;
		break;

	// Current
	case 0x10:
		T_ASSERT(length == 2);
		*(uint16_t*)outData = 0x0000;
		break;

	// Power
	case 0x18:
		T_ASSERT(length == 2);
		*(uint16_t*)outData = 0x0000;
		break;

	// State of charge
	case 0x1c:
		T_ASSERT(length == 2);
		*(uint16_t*)outData = 99;
		break;
	}
}

bool FuelGauge::tick(ICPU* cpu)
{
	return true;
}
