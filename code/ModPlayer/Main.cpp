#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "Runtime/Runtime.h"

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

	rt_console_printf("reading mod file...\n");
	if (!(file = fopen("song.mod", "rb")))
		return -1;

	fseek(file, 0, SEEK_END);
	const int32_t mod_size = ftell(file);
	rewind(file);

	rt_console_printf("size %d bytes...\n", mod_size);
	if (!(mod_data = malloc(mod_size)))
		return -1;
	else if (!fread(mod_data, mod_size, 1, file))
		return -1;
	fclose(file);

	rt_console_printf("initialize XMP...\n");

	ctx = xmp_create_context();

	rt_console_printf("preparing XMP...\n");
	xmp_load_module_from_memory(ctx, mod_data, mod_size);

	rt_console_printf("playing...\n");
	xmp_start_player(ctx, SAMPLE_RATE, 0);
	xmp_set_player(ctx, XMP_PLAYER_INTERP, XMP_INTERP_LINEAR);

	struct xmp_frame_info fi;
	uint8_t buffer[4][1024 * 2 * sizeof(int16_t)];
	int32_t i = 0;

	while (rt_input_get_state() == 0)
	{
		xmp_play_buffer(ctx, buffer[i], 1024 * 2 * sizeof(int16_t), 0);
		rt_audio_wait(1 << 0);
		rt_audio_play(0, buffer[i], 1024, RT_AUDIO_MODE_APPEND | RT_AUDIO_MODE_STEREO);
		i = (i + 1) & 3;
	}

	rt_console_printf("bye!\n");
	return 0;
}
