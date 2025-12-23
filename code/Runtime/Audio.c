/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include "Runtime/Audio.h"
#include "Runtime/I2C.h"
#include "Runtime/Input.h"
#include "Runtime/Kernel.h"

#include <HAL/Audio.h>
#include <HAL/Timer.h>

#define TLV320_ADDR 0x18

static uint32_t s_dma_tag = 0;

int32_t rt_audio_init()
{
	// Initialize TLV320DAC3100 chip.

	// Set register page 0.
	rt_i2c_write(TLV320_ADDR, 0x00, 0x00, RT_I2C_MODE_SLOW);

	// Initiate SW reset (PLL off).
	rt_i2c_write(TLV320_ADDR, 0x01, 0x01, RT_I2C_MODE_SLOW);
	hal_timer_wait_ms(200);

	// Program clock settings.
	rt_i2c_write(TLV320_ADDR, 0x04, 0b00000000, RT_I2C_MODE_SLOW);	// (PLL_clkin = MCLK, codec_clkin = MCLK).

	// Program and power up NDAC.
	rt_i2c_write(TLV320_ADDR, 0x0b, 0x80 | 4, RT_I2C_MODE_SLOW);

	// Program and power up MDAC.
	rt_i2c_write(TLV320_ADDR, 0x0c, 0x80 | 2, RT_I2C_MODE_SLOW);
		
	// Program OSR value.
	rt_i2c_write(TLV320_ADDR, 0x0d, 0x00, RT_I2C_MODE_SLOW);
	rt_i2c_write(TLV320_ADDR, 0x0e, 0x80, RT_I2C_MODE_SLOW);

	// Program I2S word length and master mode.
	rt_i2c_write(TLV320_ADDR, 0x1b, 0x00, RT_I2C_MODE_SLOW);

	// Misc page 0 controls.
	rt_i2c_write(TLV320_ADDR, 0x74, 0x00, RT_I2C_MODE_SLOW); // DAC volume control thru pin disable.

	// Set register page 1
	rt_i2c_write(TLV320_ADDR, 0x00, 0x01, RT_I2C_MODE_SLOW);

	// Program common-mode voltage (default 1.35V).
	rt_i2c_write(TLV320_ADDR, 0x1f, 0x04, RT_I2C_MODE_SLOW);

	// Program headphone depop settings
	rt_i2c_write(TLV320_ADDR, 0x21, 0x4e, RT_I2C_MODE_SLOW);

	// Program routing of DAC output to output amp.
	rt_i2c_write(TLV320_ADDR, 0x23, 0x44, RT_I2C_MODE_SLOW);

	// Unmute and set gain of output driver.
	rt_i2c_write(TLV320_ADDR, 0x28, 0x06, RT_I2C_MODE_SLOW);
	rt_i2c_write(TLV320_ADDR, 0x29, 0x06, RT_I2C_MODE_SLOW);
	// rt_i2c_write(TLV320_ADDR, 0x2a, 0x1c, RT_I2C_MODE_SLOW);	// 24dB D-class amp gain
	rt_i2c_write(TLV320_ADDR, 0x2a, 0x14, RT_I2C_MODE_SLOW);		// 18dB D-class amp gain
	rt_i2c_write(TLV320_ADDR, 0x1f, 0xc2, RT_I2C_MODE_SLOW);
	rt_i2c_write(TLV320_ADDR, 0x20, 0x86, RT_I2C_MODE_SLOW);
	rt_i2c_write(TLV320_ADDR, 0x24, 0x92, RT_I2C_MODE_SLOW);
	rt_i2c_write(TLV320_ADDR, 0x25, 0x92, RT_I2C_MODE_SLOW);
	rt_i2c_write(TLV320_ADDR, 0x26, 0x92, RT_I2C_MODE_SLOW);

	// Set register page to 0.
	rt_i2c_write(TLV320_ADDR, 0x00, 0x00, RT_I2C_MODE_SLOW);

	// Power up DAC.
	//rt_i2c_write(TLV320_ADDR, 0x3f, 0xd4);
	rt_i2c_write(TLV320_ADDR, 0x3f, 0b11111100, RT_I2C_MODE_SLOW);

	const int8_t vol = -16;
	rt_i2c_write(TLV320_ADDR, 0x41, *(uint8_t*)&vol, RT_I2C_MODE_SLOW);
	rt_i2c_write(TLV320_ADDR, 0x42, *(uint8_t*)&vol, RT_I2C_MODE_SLOW);

	// Unmute digital volume control.
	rt_i2c_write(TLV320_ADDR, 0x40, 0x00, RT_I2C_MODE_SLOW);

	// Set GPIO INT1 mode.
	// rt_i2c_write(TLV320_ADDR, 0x33, 0x24, RT_I2C_MODE_SLOW);

	// Enable headset detection.
	rt_i2c_write(TLV320_ADDR, 0x43, 0x80, RT_I2C_MODE_SLOW);

	// Select initial filter.
	rt_audio_set_filter(0);

	// Initialize audio controller.
	hal_audio_init();

	return 0;
}

void rt_audio_set_volume(uint8_t volume)
{
	// rt_i2c_write(TLV320_ADDR, 0x41, 0x00, RT_I2C_MODE_SLOW);
	// rt_i2c_write(TLV320_ADDR, 0x42, 0x00, RT_I2C_MODE_SLOW);
	// rt_i2c_write(TLV320_ADDR, 0x40, 0x00, RT_I2C_MODE_SLOW);
}

void rt_audio_set_playback_rate(uint32_t rate)
{
	hal_audio_set_playback_rate(rate);
}

void rt_audio_set_filter(uint8_t filter)
{
	if (filter >= 0 && filter < 10)
	{
		rt_i2c_write(TLV320_ADDR, 0x00, 0, RT_I2C_MODE_SLOW);
		rt_i2c_write(TLV320_ADDR, 0x3c, filter + 1, RT_I2C_MODE_SLOW);
	}
}

uint8_t rt_audio_is_channels_busy(uint32_t channel_mask)
{
	const uint32_t busy = hal_audio_get_channels_busy();
	return ((busy & channel_mask) != 0) ? 1 : 0;
}

void rt_audio_play_stereo(uint8_t channel, const void* samples, uint32_t nsamples)
{
	if (nsamples > 0)
	{
		__asm__ volatile ( "fence" );
		hal_audio_setup_channel(channel, samples, nsamples);
	}
}

void rt_audio_wait(uint32_t channel_mask)
{
	for (;;)
	{
		const uint32_t busy = hal_audio_get_channels_busy();
		if ((busy & channel_mask) == 0)
			break;
		rt_kernel_yield();
	}
}

int32_t rt_audio_headphones_connected()
{
	uint8_t hs = 0;
	rt_i2c_write(TLV320_ADDR, 0x00, 0x00, RT_I2C_MODE_SLOW);
	rt_i2c_read(TLV320_ADDR, 0x43, &hs, 1, RT_I2C_MODE_SLOW);
	return ((hs & 0b00100000) != 0) ? 1 : 0;
}
