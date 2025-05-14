#include "Runtime/HAL/SystemRegisters.h"

#define BASE (volatile uint32_t*)(SYSREG_BASE)

void sysreg_write(uint8_t reg, uint32_t value)
{
	//(BASE)[reg] = value;
}

void sysreg_modify(uint8_t reg, uint32_t mask, uint32_t value)
{
	//(BASE)[reg] = ((BASE)[reg] & ~mask) | (value & mask);
}

uint32_t sysreg_read(uint8_t reg)
{
	//return (BASE)[reg];

	switch (reg)
	{
	case SR_REG_FREQUENCY:
		return 100000000;
	case SR_REG_DEVICE_ID:
		return SR_DEVICE_ID_ICESUGAR;
	case SR_REG_RAM_SIZE:
		return 1024 * 1024;
	default:
		return 0;
	}
}
