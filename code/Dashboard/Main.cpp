#include <stdio.h>
#include <stdlib.h>

#include <HAL/UART.h>

#include "Runtime/Audio.h"
#include "Runtime/CRT.h"
#include "Runtime/File.h"
#include "Runtime/Runtime.h"
#include "Runtime/Kernel.h"


int main()
{
	runtime_init();

	const int32_t fd = file_open("SONG", FILE_MODE_READ);
	if (fd < 0)
		printf("Failed to open file!\n");

	const int32_t fs = file_size(fd);
	printf("Reading music (%d bytes)...\n", fs);
	uint16_t* ptr = (uint16_t*)malloc(fs);
	
	uint8_t* dst = (uint8_t*)ptr;
	for (int32_t i = 0; i < fs; i += 512)
	{
		runtime_update();

		printf("Reading %d...\n", i);
		file_read(fd, dst, 512);
		dst += 512;
	}

	file_close(fd);

	printf("Playing music...\n");
	int32_t offset = 0;
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
