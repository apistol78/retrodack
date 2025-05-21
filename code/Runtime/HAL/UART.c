/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include "Runtime/HAL/UART.h"

volatile uint32_t* const c_base = (volatile uint32_t*)UART_BASE;
volatile uint32_t* const c_status = (volatile uint32_t*)(UART_BASE + 0x04);

void uart_tx_u8(uint8_t data)
{
	*c_base = (uint32_t)data;
}

uint32_t uart_rx_full()
{
	return (*c_status & 0x00000001) ? 1 : 0;
}

uint32_t uart_rx_empty()
{
	return (*c_status & 0x00000002) ? 1 : 0;
}

uint8_t uart_rx_u8()
{
	return (uint8_t)*c_base;
}

uint16_t uart_rx_u16()
{
	uint16_t value = 0;
	uint8_t* tmp = (uint8_t*)&value;
	tmp[0] = uart_rx_u8();
	tmp[1] = uart_rx_u8();
	return value;
}

uint32_t uart_rx_u32()
{
	uint32_t value = 0;
	uint8_t* tmp = (uint8_t*)&value;
	tmp[0] = uart_rx_u8();
	tmp[1] = uart_rx_u8();
	tmp[2] = uart_rx_u8();
	tmp[3] = uart_rx_u8();
	return value;
}
