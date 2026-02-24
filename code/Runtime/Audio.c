/*
 RetroDÄCK
 Copyright (c) 2025-2026 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include <stdlib.h>
#include <string.h>

#include "Runtime/Runtime.h"

#include <HAL/Audio.h>
#include <HAL/Interrupt.h>
#include <HAL/Timer.h>

#define NUM_CHANNELS 2
#define TLV320_ADDR 0x18

#pragma pack(1)
struct RiffChunk
{
	uint8_t id[4];
	uint32_t size;
};

struct WaveFormat
{
	uint16_t compression;
	uint16_t channels;
	uint32_t sampleRate;
	uint32_t averageBytesPerSecond;
	uint16_t blockAlign;
	uint16_t bitsPerSample;
};
#pragma pack()

static uint32_t s_dma_tag = 0;
static volatile kernel_sig_t s_audio_signal;

static uint16_t swap8in16(uint16_t v)
{
	return
		((v & 0xff00) >> 8) |
		((v & 0x00ff) << 8);
}

static uint32_t swap8in32(uint32_t v)
{
	return
		((v & 0xff000000) >> 24) |
		((v & 0x00ff0000) >> 8) |
		((v & 0x0000ff00) << 8) |
		((v & 0x000000ff) >> 24);
}

static void audio_interrupt(uint32_t source)
{
	rt_kernel_sig_raise(&s_audio_signal);
}

int32_t rt_audio_init()
{
	// Initialize TLV320DAC3100 chip.
	rt_i2c_acquire();

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

	rt_i2c_release();

	// Select initial filter.
	rt_audio_set_filter(0);

	// Initialize audio controller.
	hal_audio_init();
	
	// Setup interrupt handler.
	rt_kernel_sig_init(&s_audio_signal);
	hal_interrupt_set_handler(IRQ_SOURCE_PLIC_1, audio_interrupt);
	return 0;
}

void rt_audio_set_volume(uint8_t volume)
{
	// rt_i2c_acquire();
	// rt_i2c_write(TLV320_ADDR, 0x41, 0x00, RT_I2C_MODE_SLOW);
	// rt_i2c_write(TLV320_ADDR, 0x42, 0x00, RT_I2C_MODE_SLOW);
	// rt_i2c_write(TLV320_ADDR, 0x40, 0x00, RT_I2C_MODE_SLOW);
	// rt_i2c_release();
}

void rt_audio_set_playback_rate(uint32_t rate)
{
	hal_audio_set_playback_rate(rate);
}

void rt_audio_set_filter(uint8_t filter)
{
	if (filter >= 0 && filter < 10)
	{
		rt_i2c_acquire();
		rt_i2c_write(TLV320_ADDR, 0x00, 0, RT_I2C_MODE_SLOW);
		rt_i2c_write(TLV320_ADDR, 0x3c, filter + 1, RT_I2C_MODE_SLOW);
		rt_i2c_release();
	}
}

uint8_t rt_audio_get_num_channels()
{
	return NUM_CHANNELS;
}

uint8_t rt_audio_is_channels_busy(uint32_t channel_mask)
{
	const uint32_t busy = hal_audio_get_channels_busy();
	return ((busy & channel_mask) != 0) ? 1 : 0;
}

void rt_audio_play(uint8_t channel, const void* samples, uint32_t nsamples, uint32_t mode)
{
	if (nsamples > 0 && channel < NUM_CHANNELS)
	{
		__asm__ volatile ( "fence" );
		rt_kernel_enter_critical();
		hal_audio_setup_channel(channel, samples, nsamples, mode);
		rt_kernel_leave_critical();
	}
}

void rt_audio_set_channel_volume(uint8_t channel, uint8_t volume)
{
	if (channel < NUM_CHANNELS)
		hal_audio_set_channel_volume(channel, volume);
}

void rt_audio_wait_all(uint32_t channel_mask)
{
	for (;;)
	{
		// First check if any channel is actually busy.
		const uint32_t busy = hal_audio_get_channels_busy();
		if ((busy & channel_mask) == 0)
			return;

		// Channels are busy; wait on interrupt.
		rt_kernel_sig_try_wait(&s_audio_signal, 100);
	}
}

void rt_audio_wait_any(uint32_t channel_mask)
{
	for (;;)
	{
		// First check if any channel is actually busy.
		const uint32_t busy = hal_audio_get_channels_busy();
		if ((busy & channel_mask) != channel_mask)
			return;

		// Channels are busy; wait on interrupt.
		rt_kernel_sig_try_wait(&s_audio_signal, 100);
	}
}

int32_t rt_audio_headphones_connected()
{
	uint8_t hs = 0;
	rt_i2c_acquire();
	rt_i2c_write(TLV320_ADDR, 0x00, 0x00, RT_I2C_MODE_SLOW);
	rt_i2c_read(TLV320_ADDR, 0x43, &hs, 1, RT_I2C_MODE_SLOW);
	rt_i2c_release();
	return ((hs & 0b00100000) != 0) ? 1 : 0;
}

rt_audio_sound_t* rt_audio_create_sound(uint32_t nsamples, uint32_t mode)
{
	const uint32_t nchannels = (mode == RT_AUDIO_MODE_MONO) ? 1 : 2;

	rt_audio_sound_t* sound = (rt_audio_sound_t*)malloc(sizeof(rt_audio_sound_t) + nsamples * nchannels * sizeof(int16_t));
	if (!sound)
		return 0;

	sound->samples = (int16_t*)(sound + 1);
	sound->nsamples = nsamples;
	sound->mode = mode;
	return sound;
}

void rt_audio_destroy_sound(rt_audio_sound_t* sound)
{
	free(sound);
}

rt_audio_sound_t* rt_audio_load_sound(const char* filename)
{
	struct RiffChunk hdr, fmt, data;
	struct WaveFormat wf;
	rt_audio_sound_t* snd;

	const int32_t fd = file_open(filename, FILE_MODE_READ);
	if (fd <= 0)
		return 0;

	file_read(fd, &hdr, sizeof(hdr));
	hdr.size = swap8in32(hdr.size);

	file_read(fd, &fmt, sizeof(fmt));
	fmt.size = swap8in32(fmt.size);

	file_read(fd, &wf, sizeof(wf));
	wf.compression = swap8in16(wf.compression);
	wf.channels = swap8in16(wf.channels);
	wf.sampleRate = swap8in32(wf.sampleRate);
	wf.averageBytesPerSecond = swap8in32(wf.averageBytesPerSecond);
	wf.blockAlign = swap8in16(wf.blockAlign);
	wf.bitsPerSample = swap8in16(wf.bitsPerSample);

	if (wf.channels != 1 && wf.channels != 2)
	{
		file_close(fd);
		return 0;
	}

	file_seek(fd, fmt.size - sizeof(wf), FILE_SEEK_CUR);

	for (;;)
	{
		if (file_read(fd, &data, sizeof(data)) != sizeof(data))
		{
			file_close(fd);
			return 0;		
		}
		if (memcmp(data.id, "data", 4U) == 0)
			break;
		file_seek(fd, data.size, FILE_SEEK_CUR);
	}

	const uint32_t nsamples = data.size / (wf.channels * wf.bitsPerSample / 8);

	snd = rt_audio_create_sound(nsamples, (wf.channels == 1) ? RT_AUDIO_MODE_MONO : RT_AUDIO_MODE_STEREO);
	if (!snd)
	{
		file_close(fd);
		return 0;
	}

	int16_t* wp = snd->samples;
	for (uint32_t s = 0; s < nsamples; ++s)
	{
		for (uint16_t i = 0; i < wf.channels; ++i)
		{
			switch (wf.bitsPerSample)
			{
			case 8:
				{
					uint8_t smp;
					file_read(fd, &smp, 1);
					*wp++ = (int16_t)(((smp / 255.0f) * 2.0f - 1.0f) * 32767.0f);
				}
				break;

			case 16:
				{
					int16_t smp;
					file_read(fd, &smp, 2);
					*wp++ = smp;
				}
				break;
			}
		}
	}

	file_close(fd);
	return snd;
}

void rt_audio_play_sound(uint8_t channel, const rt_audio_sound_t* snd, uint32_t mode)
{
	rt_kernel_enter_critical();
	hal_audio_setup_channel(channel, snd->samples, snd->nsamples, mode | snd->mode);
	rt_kernel_leave_critical();
}
