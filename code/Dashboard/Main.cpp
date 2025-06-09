#include <stdio.h>
#include <stdlib.h>

#include <HAL/Audio.h>
#include <HAL/Video.h>

#include "Runtime/File.h"
#include "Runtime/Runtime.h"
#include "Runtime/Kernel.h"


void __register_exitproc(void) {}
void __call_exitprocs(void) {}

int main()
{
	runtime_init();
	
	// kernel_cs_init(&lock);
	// kernel_create_thread(&thread_1);
	// kernel_create_thread(&thread_2);

	for (int i = 0; i < 256; ++i)
	{
		hal_video_set_palette(
			i,
			(rand() & 255) | ((rand() & 255) << 8) | ((rand() & 255) << 16)
		);
	}

	int32_t fd = file_open("SPLASH", FILE_MODE_READ);
	
	for (int i = 0; i < 256; ++i)
	{
		uint32_t clr;
		file_read(fd, (uint8_t*)&clr, 4);
		hal_video_set_palette(i, clr);
	}

	void* fb = hal_video_get_secondary_target();
	file_read(fd, (uint8_t*)fb, 720 * 720);

	file_close(fd);

	hal_video_present();

	for (;;)
	{
	}


	return 0;
}