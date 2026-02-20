#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "Runtime/Runtime.h"

#include <xmp.h>

#define SAMPLE_RATE 22050

xmp_context ctx;
uint8_t C = 0;


inline int16_t max(int16_t a, int16_t b)
{
	return (a > b) ? a : b;
}

static void draw_thread()
{
	const int32_t FW = 360;
	const int32_t FH = 360;

	for (;;)
	{
		rt_gfx_context_t ctx;
		ctx.width = FW;
		ctx.height = FH;
		ctx.pixels = (uint8_t*)rt_video_get_secondary_target();

		rt_video_clear(0);
		rt_video_wait();		

		const int32_t w = C;
		rt_gfx_fill_rect(&ctx, 180 - w, 180 - w, w * 2, w * 2, 1);

		rt_video_present(1);
	}
}

int main()
{
	FILE *file;
	uint8_t *mod_data;

	runtime_init();
	rt_video_set_mode(VMODE_360_360_8);
	rt_video_set_palette(0, 0x2f7dad);
	rt_video_set_palette(1, 0x9ad2f5);

	rt_kernel_create_thread(draw_thread, "draw");

	rt_audio_set_playback_rate(SAMPLE_RATE);

	if (!(file = fopen("song.mod", "rb")))
		return -1;

	fseek(file, 0, SEEK_END);
	const int32_t mod_size = ftell(file);
	rewind(file);

	if (!(mod_data = (uint8_t*)malloc(mod_size)))
		return -1;
	else if (!fread(mod_data, mod_size, 1, file))
		return -1;
	fclose(file);

	ctx = xmp_create_context();

	xmp_load_module_from_memory(ctx, mod_data, mod_size);

	xmp_start_player(ctx, SAMPLE_RATE, 0);
	xmp_set_player(ctx, XMP_PLAYER_INTERP, XMP_INTERP_LINEAR);

	struct xmp_frame_info fi;
	uint8_t buffer[4][1024 * 2 * sizeof(int16_t)];
	int32_t i = 0;

	while (rt_input_get_state() == 0)
	{
		xmp_play_buffer(ctx, buffer[i], 1024 * 2 * sizeof(int16_t), 0);

		int16_t bmx = 0;
		const int16_t* ptr = (const int16_t*)buffer[i];
		for (int32_t j = 0; j < 1024; ++j)
		{
			const int16_t mx = max(abs(ptr[i * 2 + 0]), abs(ptr[i * 2 + 1]));
			bmx = max(mx, bmx);
		}

		const uint8_t ubmx = ((uint16_t)bmx) >> 8;
		if (ubmx > C)
			C = ubmx;
		else
			C -= (C > 10) ? 10 : C;

		rt_input_set_tb_color(C);

		rt_audio_wait_any(1 << 0);
		rt_audio_play(0, buffer[i], 1024, RT_AUDIO_MODE_APPEND | RT_AUDIO_MODE_STEREO);
		i = (i + 1) & 3;
	}


	return 0;
}
