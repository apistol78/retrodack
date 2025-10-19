#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "Runtime/Audio.h"
#include "Runtime/Console.h"
#include "Runtime/CRT.h"
#include "Runtime/File.h"
#include "Runtime/Runtime.h"
#include "Runtime/Timer.h"
#include "Runtime/Kernel.h"

#include <xmp.h>

xmp_context ctx;

int main()
{
	FILE *file;
	char *mod_data;

#define SAMPLE_RATE 22050

	runtime_init();
	rt_audio_set_playback_rate(SAMPLE_RATE);
	rt_console_init();


	printf("reading mod file...\n");
	if (!(file = fopen("2nd_pm.s3m", "rb")))
	{
		//printf("error: can't open '%s' for reading\n", argv[1]);
		return -1;
	}
	fseek(file, 0, SEEK_END);
	const int32_t mod_size = ftell(file);
	rewind(file);
	printf("size %d bytes...\n", mod_size);
	if (!(mod_data = malloc(mod_size)))
	{
		rt_console_printf("error: %d-byte memory allocation failed\n", mod_size);
		return -1;
	}
	else if (!fread(mod_data, mod_size, 1, file))
	{
		//printf("error: error reading file '%s'\n", argv[1]);
		return -1;
	}
	fclose(file);



	printf("initialize XMP...\n");

	ctx = xmp_create_context();

	printf("preparing XMP...\n");
	// xmp_load_module(ctx, "song.mod");
	xmp_load_module_from_memory(ctx, mod_data, mod_size);


	printf("playing...\n");
	xmp_start_player(ctx, SAMPLE_RATE, 0);
	xmp_set_player(ctx, XMP_PLAYER_INTERP, XMP_INTERP_NEAREST);
	xmp_set_player(ctx, XMP_PLAYER_DSP, 0);

	struct xmp_frame_info fi;

	rt_kernel_enter_critical();


	
	uint8_t buffer[256][1024 * 2 * sizeof(int16_t)];
	int32_t i = 0;

	for (;;)
	{
		

		// if (xmp_play_frame(ctx) != 0)
		// 	break;

		// xmp_get_frame_info(ctx, &fi);

		// rt_audio_wait();
		// rt_audio_play_stereo(fi.buffer, fi.buffer_size >> 2);


		xmp_play_buffer(ctx, buffer[i], 1024 * 2 * sizeof(int16_t), 0);
		rt_audio_wait();
		rt_audio_play_stereo(buffer[i], 1024);

		i = (i + 1) & 255;



		// uint32_t t1 = rt_timer_get_ms();
		// if ((t1 - t0) > 4000)
		// {
		// 	printf("filter %d\n", filter);
		// 	rt_audio_set_filter(filter);
		// 	filter = (filter + 1) % 11;
		// 	t0 = t1;
		// }
	}

	return 0;
}
