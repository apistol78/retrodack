#pragma once

#include <Emulator2/Devices/I2C.h>

class RTC : public I2C::ISlave
{
	T_RTTI_CLASS;

public:
	virtual void write(uint8_t controlAddr, uint8_t data) override final;

	virtual void read(uint8_t controlAddr, uint8_t length, uint8_t* outData) override final;

	virtual bool tick(ICPU* cpu) override final;
};
