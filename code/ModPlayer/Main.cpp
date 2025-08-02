#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "Runtime/Audio.h"
#include "Runtime/Console.h"
#include "Runtime/CRT.h"
#include "Runtime/File.h"
#include "Runtime/Runtime.h"
#include "Runtime/Kernel.h"

#define POCKETMOD_IMPLEMENTATION
#define POCKETMOD_NO_INTERPOLATION
#include "pocketmod.h"

#define SAMPLE_RATE 44100/4

pocketmod_context context;

void thread_player()
{
	float buffer[16000][2];
	int16_t output[16000];

	//while (pocketmod_loop_count(&context) == 0)
	for (;;)
	{
		int32_t avail = 4096 - rt_audio_get_queued();
		
		int32_t rendered_bytes = pocketmod_render(&context, buffer, avail * sizeof(float[2]));
		int32_t rendered_samples = rendered_bytes / sizeof(float[2]);

		for (int32_t i = 0; i < rendered_samples; i++)
		{
			output[i] = (int16_t) (
				(buffer[i][0] + buffer[i][1]) * 0x7fff
			);
		}

		rt_audio_play_mono(output, rendered_samples);
	}
}

int main()
{
	FILE *file;
	char *mod_data;

	runtime_init();
	rt_console_init();

	rt_console_printf("reading mod file...\n");

	if (!(file = fopen("song.mod", "rb")))
	{
		//printf("error: can't open '%s' for reading\n", argv[1]);
		return -1;
	}
	
	fseek(file, 0, SEEK_END);
	const int32_t mod_size = ftell(file);
	rewind(file);

	rt_console_printf("size %d bytes...\n", mod_size);

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

	rt_console_printf("initialize pocketmod...\n");

	/* Initialize the renderer */
	if (!pocketmod_init(&context, mod_data, mod_size, SAMPLE_RATE))
	{
		//printf("error: '%s' is not a valid MOD file\n", argv[1]);
		return -1;
	}

	rt_console_printf("playing...\n");

	kernel_create_thread(thread_player);

	for (;;)
	{
		kernel_sleep(1000);
		rt_console_printf(".");
	}

	return 0;
}
