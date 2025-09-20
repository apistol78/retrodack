/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include "Runtime/I2C.h"
#include "Runtime/Input.h"
#include "Runtime/Kernel.h"

#include <HAL/Audio.h>
#include <HAL/DMA.h>
#include <HAL/Timer.h>

#define TLV320_ADDR 0x18

static uint32_t s_dma_tag = 0;

int32_t rt_audio_init()
{
	// Initialize TLV320DAC3100 chip.

	// Set register page 0.
	rt_i2c_write(TLV320_ADDR, 0x00, 0x00);

	// Initiate SW reset (PLL off).
	rt_i2c_write(TLV320_ADDR, 0x01, 0x01);
	hal_timer_wait_ms(200);

	// Program clock settings.
	rt_i2c_write(TLV320_ADDR, 0x04, 0x03);	// (PLL_clkin = MCLK, codec_clkin = PLL_CLK).
	rt_i2c_write(TLV320_ADDR, 0x06, 0x08);
	rt_i2c_write(TLV320_ADDR, 0x07, 0x00);
	rt_i2c_write(TLV320_ADDR, 0x08, 0x00);

	// Power up PLL.
	rt_i2c_write(TLV320_ADDR, 0x05, 0x91);

	// Program and power up NDAC.
	rt_i2c_write(TLV320_ADDR, 0x0b, 0x88);

	// Program and power up MDAC.
	rt_i2c_write(TLV320_ADDR, 0x0c, 0x82);

	// Program OSR value.
	rt_i2c_write(TLV320_ADDR, 0x0d, 0x00);
	rt_i2c_write(TLV320_ADDR, 0x0e, 0x80);

	// Program I2S word length and master mode.
	rt_i2c_write(TLV320_ADDR, 0x1b, 0x00);

	// Program the processing block to be used (PRB_P11).
	rt_i2c_write(TLV320_ADDR, 0x3c, 0x0b);
	rt_i2c_write(TLV320_ADDR, 0x00, 0x08);
	rt_i2c_write(TLV320_ADDR, 0x01, 0x04);
	rt_i2c_write(TLV320_ADDR, 0x00, 0x00);

	// Misc page 0 controls.
	rt_i2c_write(TLV320_ADDR, 0x74, 0x00); // DAC volume control thru pin disable.

	// Set register page 1
	rt_i2c_write(TLV320_ADDR, 0x00, 0x01);

	// Program common-mode voltage (default 1.35V).
	rt_i2c_write(TLV320_ADDR, 0x1f, 0x04);

	// Program headphone depop settings
	rt_i2c_write(TLV320_ADDR, 0x21, 0x4e);

	// Program routing of DAC output to output amp.
	rt_i2c_write(TLV320_ADDR, 0x23, 0x44);

	// Unmute and set gain of output driver.
	rt_i2c_write(TLV320_ADDR, 0x28, 0x06);
	rt_i2c_write(TLV320_ADDR, 0x29, 0x06);
	// rt_i2c_write(TLV320_ADDR, 0x2a, 0x1c);	// 24dB D-class amp gain
	rt_i2c_write(TLV320_ADDR, 0x2a, 0x14);		// 18dB D-class amp gain
	rt_i2c_write(TLV320_ADDR, 0x1f, 0xc2);
	rt_i2c_write(TLV320_ADDR, 0x20, 0x86);
	rt_i2c_write(TLV320_ADDR, 0x24, 0x92);
	rt_i2c_write(TLV320_ADDR, 0x25, 0x92);
	rt_i2c_write(TLV320_ADDR, 0x26, 0x92);

	// Set register page to 0.
	rt_i2c_write(TLV320_ADDR, 0x00, 0x00);

	// Power up DAC and set digital gain.
	rt_i2c_write(TLV320_ADDR, 0x3f, 0xd4);

	const int8_t vol = -16;
	rt_i2c_write(TLV320_ADDR, 0x41, *(uint8_t*)&vol);
	rt_i2c_write(TLV320_ADDR, 0x42, *(uint8_t*)&vol);

	// Unmute digital volume control.
	rt_i2c_write(TLV320_ADDR, 0x40, 0x00);

	// Set GPIO INT1 mode.
	// rt_i2c_write(TLV320_ADDR, 0x33, 0x24);

	// Enable headset detection.
	rt_i2c_write(TLV320_ADDR, 0x43, 0x80);

	// Initialize audio controller.
	hal_audio_init();
	return 0;
}

void rt_audio_set_volume(uint8_t volume)
{
	// rt_i2c_write(TLV320_ADDR, 0x41, 0x00);
	// rt_i2c_write(TLV320_ADDR, 0x42, 0x00);
	// rt_i2c_write(TLV320_ADDR, 0x40, 0x00);
}

void rt_audio_set_playback_rate(uint32_t rate)
{
	hal_audio_set_playback_rate(rate);
}

uint32_t rt_audio_get_queued()
{
	return hal_audio_get_queued();
}

void rt_audio_play_stereo(const void* samples, uint32_t nsamples)
{
	__asm__ volatile ( "fence" );
	s_dma_tag = hal_dma_feed((void*)AUDIO_BASE, samples, nsamples);
}

void rt_audio_wait()
{
	while (hal_dma_retired() < s_dma_tag)
		rt_kernel_yield();
}
