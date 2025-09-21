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

#define SAMPLE_RATE 22050

pocketmod_context context;

void thread_player()
{
	float buffer[1024][2];
	uint32_t output[32][4096];
	int32_t count = 0;

	for (;;)
	{
		int32_t rendered_bytes = pocketmod_render(&context, buffer, sizeof(buffer));
		int32_t rendered_samples = rendered_bytes / sizeof(float[2]);

		uint32_t* ptr = output[count];

		for (int32_t i = 0; i < rendered_samples; i++)
		{
			const int16_t lh = (int16_t)(buffer[i][0] * 0x7fff);
			const int16_t rh = (int16_t)(buffer[i][1] * 0x7fff);

			const uint32_t ulh = *(const uint16_t*)&lh;
			const uint32_t urh = *(const uint16_t*)&rh;

			ptr[i] = (ulh << 16) | urh;
		}
		
		rt_audio_wait();
		rt_audio_play_stereo(ptr, rendered_samples);

		count = (count + 1) & 31;
	}
}

int main()
{
	FILE *file;
	char *mod_data;

	runtime_init();
	rt_audio_set_playback_rate(SAMPLE_RATE);
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

	// rt_kernel_create_thread(thread_player);

	for (;;)
	{
		rt_kernel_sleep(1000);
		rt_console_printf(".");
	}

	return 0;
}
