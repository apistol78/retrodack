#include <stdio.h>
#include <stdlib.h>

#include <HAL/Audio.h>
#include <HAL/Video.h>
#include <HAL/Timer.h>

#include "Runtime/Audio.h"
#include "Runtime/CRT.h"
#include "Runtime/File.h"
#include "Runtime/Runtime.h"
#include "Runtime/Kernel.h"


int main()
{
	runtime_init();
	
	// kernel_cs_init(&lock);
	// kernel_create_thread(&thread_1);
	// kernel_create_thread(&thread_2);

	const int32_t fd = file_open("SPLASH", FILE_MODE_READ);
	
	uint8_t pal[256 * 4];
	file_read(fd, pal, 256 * 4);

	for (int i = 0; i < 256; ++i)
		hal_video_set_palette(i, 0);

	void* fb = hal_video_get_secondary_target();
	file_read(fd, fb, 720 * 720);
	file_close(fd);

	hal_video_present();


	uint8_t out[256 * 4];
	for (int i = 0; i < 256; ++i)
	{
		for (int j = 0; j < 256 * 4; ++j)
			out[j] = (pal[j] * i) >> 8;
		
		for (int j = 0; j < 256; ++j)
			hal_video_set_palette(j, *(uint32_t*)&out[j * 4]);

		hal_timer_wait_ms(30);
	}


	for (;;)
	{
		printf("... play %d...\n", offset);

		rt_audio_play_mono(&ptr[offset], 1024);

		offset += 1024;
		if (offset * 2 >= fs)
			offset = 0;
	}

	return 0;
}
