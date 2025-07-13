#pragma once

#include <functional>

#include <Emulator2/Devices/I2C.h>

class TrackBallDevice : public I2C::ISlave
{
	T_RTTI_CLASS;

public:
	virtual void write(uint8_t controlAddr, uint8_t data) override final;

	virtual void read(uint8_t controlAddr, uint8_t length, uint8_t* outData) override final;

	virtual bool tick(ICPU* cpu) override final;

	void setCallback(const std::function< void() >& fn);

	void accumulateMovement(int32_t dx, int32_t dy);

	void setButton(bool pressed);

private:
	std::function< void() > m_callback;
	int32_t m_deltaX = 0;
	int32_t m_deltaY = 0;
	int32_t m_absX = 0;
	int32_t m_absY = 0;
	bool m_pressed = false;
	bool m_trig = false;
};
