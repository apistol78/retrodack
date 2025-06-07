/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include "Runtime/Input.h"

#include <HAL/Audio.h>
#include <HAL/I2C.h>
#include <HAL/Timer.h>

#define TLV320_ADDR 0x18

int32_t rt_audio_init()
{
	// Initialize TLV320DAC3100 chip.

	// Set register page 0.
	hal_i2c_write(TLV320_ADDR, 0x00, 0x00);

	// Initiate SW reset (PLL off).
	hal_i2c_write(TLV320_ADDR, 0x01, 0x01);
	hal_timer_wait_ms(200);

	// Program clock settings (PLL_clkin = MCLK, codec_clkin = PLL_CLK).
	hal_i2c_write(TLV320_ADDR, 0x04, 0x03);
	hal_i2c_write(TLV320_ADDR, 0x06, 0x08);
	hal_i2c_write(TLV320_ADDR, 0x07, 0x00);
	hal_i2c_write(TLV320_ADDR, 0x08, 0x00);

	// Power up PLL.
	hal_i2c_write(TLV320_ADDR, 0x05, 0x91);

	// Program and power up NDAC.
	hal_i2c_write(TLV320_ADDR, 0x0b, 0x88);

	// Program and power up MDAC.
	hal_i2c_write(TLV320_ADDR, 0x0c, 0x82);

	// Program OSR value.
	hal_i2c_write(TLV320_ADDR, 0x0d, 0x00);
	hal_i2c_write(TLV320_ADDR, 0x0e, 0x80);

	// Program I2S word length and master mode.
	hal_i2c_write(TLV320_ADDR, 0x1b, 0x00);

	// Program the processing block to be used (PRB_P11).
	hal_i2c_write(TLV320_ADDR, 0x3c, 0x0b);
	hal_i2c_write(TLV320_ADDR, 0x00, 0x08);
	hal_i2c_write(TLV320_ADDR, 0x01, 0x04);
	hal_i2c_write(TLV320_ADDR, 0x00, 0x00);

	// Misc page 0 controls.
	hal_i2c_write(TLV320_ADDR, 0x74, 0x00); // DAC volume control thru pin disable.

	// Set register page 1
	hal_i2c_write(TLV320_ADDR, 0x00, 0x01);

	// Program common-mode volate (default 1.35V).
	hal_i2c_write(TLV320_ADDR, 0x1f, 0x04);

	// Program headphone depop settings
	hal_i2c_write(TLV320_ADDR, 0x21, 0x4e);

	// Program routing of DAC output to output amp.
	hal_i2c_write(TLV320_ADDR, 0x23, 0x44);

	// Unmute and set gain of output driver.
	hal_i2c_write(TLV320_ADDR, 0x28, 0x06);
	hal_i2c_write(TLV320_ADDR, 0x29, 0x06);
	hal_i2c_write(TLV320_ADDR, 0x2a, 0x1c);
	hal_i2c_write(TLV320_ADDR, 0x1f, 0xc2);
	hal_i2c_write(TLV320_ADDR, 0x20, 0x86);
	hal_i2c_write(TLV320_ADDR, 0x24, 0x92);
	hal_i2c_write(TLV320_ADDR, 0x25, 0x92);
	hal_i2c_write(TLV320_ADDR, 0x26, 0x92);

	// Set register page to 0.
	hal_i2c_write(TLV320_ADDR, 0x00, 0x00);

	// Power up DAC and set digital gain.
	hal_i2c_write(TLV320_ADDR, 0x3f, 0xd4);
	hal_i2c_write(TLV320_ADDR, 0x41, 0xd4);
	hal_i2c_write(TLV320_ADDR, 0x42, 0xd4);

	// Unmute digital volume control.
	hal_i2c_write(TLV320_ADDR, 0x40, 0x00);

	// Set GPIO INT1 mode.
	// hal_i2c_write(TLV320_ADDR, 0x33, 0x24);

	// Set left channel volume.
	hal_i2c_write(TLV320_ADDR, 0x41, 0x00);
	hal_i2c_write(TLV320_ADDR, 0x42, 0x00);
	hal_i2c_write(TLV320_ADDR, 0x40, 0x00);

	// Initialize audio controller.
	hal_audio_init();
	hal_audio_set_playback_rate(44100);
	return 0;
}

void rt_audio_set_volume(uint8_t volume)
{
}

uint32_t rt_audio_get_queued()
{
	return hal_audio_get_queued();
}

void rt_audio_play_mono(const int16_t* samples, uint32_t nsamples)
{
	hal_audio_play_mono(samples, nsamples);
}

void rt_audio_play_stereo(const int16_t* samples, uint32_t nsamples)
{
	hal_audio_play_stereo(samples, nsamples);
}
