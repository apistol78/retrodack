#include <stdio.h>
#include <stdlib.h>

#include <HAL/Audio.h>
#include <HAL/Video.h>
#include <HAL/Timer.h>
#include <HAL/SD.h>
#include <HAL/I2C.h>
#include <HAL/Interrupt.h>

#include "Runtime/Audio.h"
#include "Runtime/CRT.h"
#include "Runtime/File.h"
#include "Runtime/Runtime.h"
#include "Runtime/Kernel.h"



int32_t s_count = 0;

static void gpio_input_interrupt(uint32_t source)
{
	++s_count;
}


int main()
{
	crt_init();

	hal_interrupt_init();
	hal_interrupt_set_handler(IRQ_SOURCE_PLIC_1, gpio_input_interrupt);

	int32_t i = 0;
	for (;;)
	{
		uint8_t data[2] = { 0, 0 };
		hal_i2c_read(0x20, 0x00, data, 2);

		printf("%4d: %02x:%02x (%d)\n", i, data[0], data[1], s_count);

		++i;

	}


/*
	printf("init sd...\n");
	hal_sd_init(SD_MODE_SW);

	printf("reading first block...\n");
	uint8_t buf[512];
	hal_sd_read_block512(0, buf, 512);

	for (int32_t i = 0; i < 512; )
	{
		for (int32_t j = 0; j < 16; ++j, ++i)
		{
			printf("%02x ", buf[i]);
		}
		printf("\n");
	}
*/

	for (;;)
	{
		uint32_t ms = hal_timer_get_ms();
		printf("%d\n", ms);

		// uint32_t ms = rt_auto_get_queued();
	}


	for (;;);



	runtime_init();
	
	// kernel_cs_init(&lock);
	// kernel_create_thread(&thread_1);
	// kernel_create_thread(&thread_2);

	printf("enum disk...\n");
	file_enumerate("", 0, [](void* user, const char* filename, uint32_t size, uint8_t directory){
		printf("\"%s\"\n", filename);
	});



	hal_video_set_mode(VMODE_320_200_8);


/*
	const int32_t fd = file_open("SPLASH", FILE_MODE_READ);
	
	uint8_t pal[256 * 4];
	file_read(fd, pal, 256 * 4);


	for (int i = 0; i < 256; ++i)
		hal_video_set_palette(i, 0);

	void* fb = hal_video_get_primary_target(); //hal_video_get_secondary_target();
	file_read(fd, fb, 720 * 720);
	file_close(fd);

	// hal_video_present();

	uint8_t out[256 * 4];
	for (int i = 0; i < 256; ++i)
	{
		for (int j = 0; j < 256 * 4; ++j)
			out[j] = (pal[j] * i) >> 8;
		
		for (int j = 0; j < 256; ++j)
			hal_video_set_palette(j, *(uint32_t*)&out[j * 4]);

		hal_timer_wait_ms(30);
	}
*/

	for (int i = 0; i < 256; ++i)
	{
		uint32_t r = rand();
		uint32_t g = rand();
		uint32_t b = rand();
		hal_video_set_palette(i, (r << 16) | (g << 8) | b);
	}

	volatile uint8_t* fb = (volatile uint8_t*)hal_video_get_primary_target();
	for (uint32_t i = 0; i < 360 * 360; ++i)
	{
		fb[i] = rand();
	}


/*
	hal_video_set_palette(0, 0x00000000);
	hal_video_set_palette(1, 0xffffffff);
	
	for (int i = 0;; ++i)
	{
		volatile uint32_t* fb = (volatile uint32_t*)hal_video_get_secondary_target();
		for (int32_t i = 0; i < 720 * 720 / 4; ++i)
		{
			fb[i] = 0x00010001;
		}
		hal_video_present();
	}
*/


	for (int32_t i = 0;; ++i)
	{
		// printf("... play %d...\n", offset);

		// printf("playing %d...\n", i);
		// rt_audio_play_mono(ptr, 1024);

		// offset += 1024;
		// if (offset * 2 >= fs)
		// 	offset = 0;
	}

	return 0;
}
