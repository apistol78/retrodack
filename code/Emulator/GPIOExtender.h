#pragma once

#include <functional>

#include <Emulator2/Devices/I2C.h>

class GPIOExtender : public I2C::ISlave
{
	T_RTTI_CLASS;

public:
	virtual void write(uint8_t controlAddr, uint8_t data) override final;

	virtual void read(uint8_t controlAddr, uint8_t length, uint8_t* outData) override final;

	virtual bool tick(ICPU* cpu) override final;

	void setCallback(const std::function< void() >& fn);

    void setInput(uint16_t bits);

    void setInputBit(uint8_t index, bool value);

private:
	std::function< void() > m_callback;
    uint16_t m_bits = 0;
    bool m_trig = false;
};
