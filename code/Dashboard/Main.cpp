#include <stack>
#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <HAL/Audio.h>
#include <HAL/Video.h>

#include "Runtime/File.h"
#include "Runtime/Runtime.h"
#include "Runtime/Kernel.h"


bool read_image()
{
	uint8_t* framebuffer = (uint8_t*)hal_video_get_secondary_target();

	const int32_t fd = file_open("Background", FILE_MODE_READ);
	file_read(fd, framebuffer, 720 * 720);
	file_close(fd);

	return true;
}



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
	
    // float head = 0.0f;
    // float pitch = 0.0f;
	// ivec2_t sv[8];

	read_image();
	hal_video_present();


	float a = 0.0f;

    for (;;)
    {
		int16_t sm[128];
		for (int i = 0; i < 128; ++i)
		{
			sm[i] = (int16_t)(sin(a) * 15000.0f);
			a += 6.28f / 128.0f;
		}

		hal_audio_play_mono(sm, 128);

		/*
        hal_video_clear(0);

        framebuffer = (uint8_t*)hal_video_get_secondary_target();

		const float ca = cos(head);
		const float sa = sin(head);
		const float cp = cos(pitch);
		const float sp = sin(pitch);

		for (int32_t i = 0; i < 8; ++i)
		{
			float xa = vertices[i].x * ca + vertices[i].z * sa;
			float ya = vertices[i].y;
			float za = vertices[i].x * sa - vertices[i].z * ca;

			float x = xa;
			float y = ya * cp + za * sp;
			float z = ya * sp - za * cp;

			z += 5.0f;

			float w = z * 0.5f;
			float ndx = x / w;
			float ndy = y / w;

			sv[i].x = (int32_t)((ndx * 250) + 360);
			sv[i].y = (int32_t)((ndy * 250) + 360);
		}

		for (int32_t i = 0; i < (sizeof(indices) / sizeof(indices[0])) / 3; ++i)
		{
			int32_t i0 = indices[i * 3 + 0];
			int32_t i1 = indices[i * 3 + 1];
			int32_t i2 = indices[i * 3 + 2];

			triangle(
				&sv[i0],
				&sv[i2],
				&sv[i1],
				i + 1
			);			
		}

        hal_video_present();

		head += 0.01f * 2;
        pitch += 0.0165f * 2;
		*/
    }


    return 0;
}