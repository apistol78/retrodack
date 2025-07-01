#include <Core/Log/Log.h>

#include "Emulator/GPIOExtender.h"

using namespace traktor;

T_IMPLEMENT_RTTI_CLASS(L"GPIOExtender", GPIOExtender, I2C::ISlave)

void GPIOExtender::write(uint8_t controlAddr, uint8_t data)
{
}

void GPIOExtender::read(uint8_t controlAddr, uint8_t length, uint8_t* outData)
{
	if (controlAddr == 0x00)
	{
		outData[0] = m_bits & 0xff;
		outData[1] = (m_bits >> 8) & 0xff;
	}
}

bool GPIOExtender::tick(CPU* cpu)
{
	if (m_trig)
	{
		if (m_callback)
			m_callback();
		m_trig = false;
	}
	return true;
}

void GPIOExtender::setCallback(const std::function< void() >& fn)
{
	m_callback = fn;
}

void GPIOExtender::setInput(uint16_t bits)
{
    if (bits != m_bits)
    {
        m_bits = bits;
        m_trig = true;
    }
}

void GPIOExtender::setInputBit(uint8_t index, bool value)
{
    uint16_t bits = m_bits;
    if (value)
        bits |= 1 << index;
    else
        bits &= ~(1 << index);
    setInput(bits);
}