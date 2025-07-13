#include <Core/Log/Log.h>

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
	else if (controlAddr == 0x04)
	{
		outData[0] = (m_deltaX < 0) ? -m_deltaX : 0;	// left
		outData[1] = (m_deltaX > 0) ? m_deltaX : 0;	// right
		outData[2] = (m_deltaY < 0) ? -m_deltaY : 0;	// up
		outData[3] = (m_deltaY > 0) ? m_deltaY : 0;	// down
		outData[4] = m_pressed ? 0x01 : 0x00;	// switch

		m_deltaX = 0;
		m_deltaY = 0;
	}
}

bool TrackBallDevice::tick(ICPU* cpu)
{
	if (m_trig)
	{
		if (m_callback)
			m_callback();
		m_trig = false;
	}
	return true;
}

void TrackBallDevice::setCallback(const std::function< void() >& fn)
{
	m_callback = fn;
}

void TrackBallDevice::accumulateMovement(int32_t dx, int32_t dy)
{
	m_deltaX += dx;
	m_deltaY += dy;
	m_absX += dx;
	m_absY += dy;

	// Issue interrupt if there has been movement.
	if (dx != 0 && dy != 0)
		m_trig = true;
}

void TrackBallDevice::setButton(bool pressed)
{
	if (pressed != m_pressed)
	{
		m_pressed = pressed;
		m_trig = true;
	}
}