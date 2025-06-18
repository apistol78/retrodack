#include "Emulator/TrackBallDevice.h"

using namespace traktor;

T_IMPLEMENT_RTTI_CLASS(L"TrackBallDevice", TrackBallDevice, I2C::ISlave)

void TrackBallDevice::write(uint8_t controlAddr, uint8_t data)
{
}

void TrackBallDevice::read(uint8_t controlAddr, uint8_t length, uint8_t* outData)
{
	if (controlAddr == 0xfa)
	{
		outData[0] = 0x11;
		outData[1] = 0xba;
	}
}
