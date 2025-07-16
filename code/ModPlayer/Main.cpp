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

#define POCKETMOD_IMPLEMENTATION
#define POCKETMOD_NO_INTERPOLATION
#include "pocketmod.h"

#define SAMPLE_RATE 44100

/* Clip a floating point sample to the [-1, +1] range */
static float clip(float value)
{
    //value = value < -1.0f ? -1.0f : value;
    //value = value > +1.0f ? +1.0f : value;
    return value;
}

int main()
{
    pocketmod_context context;
    char *mod_data, *slash;
    int i, mod_size, samples = 0;
    clock_t time_now, time_prev = 0;
    FILE *file;

	runtime_init();

   /* Read the input file into a heap block */
    if (!(file = fopen("song.mod", "rb"))) {
        //printf("error: can't open '%s' for reading\n", argv[1]);
        return -1;
    }
    fseek(file, 0, SEEK_END);
    mod_size = ftell(file);
    rewind(file);
    if (!(mod_data = malloc(mod_size))) {
        printf("error: %d-byte memory allocation failed\n", mod_size);
        return -1;
    } else if (!fread(mod_data, mod_size, 1, file)) {
        //printf("error: error reading file '%s'\n", argv[1]);
        return -1;
    }
    fclose(file);

    /* Initialize the renderer */
    if (!pocketmod_init(&context, mod_data, mod_size, SAMPLE_RATE)) {
        //printf("error: '%s' is not a valid MOD file\n", argv[1]);
        return -1;
    }

   /* Write sample data */
    float buffer[16000][2];
    int16_t output[16000];
    while (pocketmod_loop_count(&context) == 0) {

		int32_t avail = 4096 - rt_audio_get_queued();
		
        int rendered_bytes = pocketmod_render(&context, buffer, avail * sizeof(float[2]));
        int rendered_samples = rendered_bytes / sizeof(float[2]);

        /* Convert the sample data to 16-bit and write it to the file */
        for (i = 0; i < rendered_samples; i++)
		{
            output[i] = (int16_t) (
				clip(buffer[i][0] + buffer[i][1]) * 0x7fff
			);
        }

		//printf("%d\n", rendered_samples);

		rt_audio_play_mono(output, rendered_samples);

    }

	return 0;
}
