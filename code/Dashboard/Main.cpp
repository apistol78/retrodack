#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <HAL/Audio.h>
#include <HAL/Video.h>
#include <HAL/SD.h>
#include <HAL/I2C.h>
#include <HAL/Interrupt.h>

#include "Runtime/Audio.h"
#include "Runtime/CRT.h"
#include "Runtime/File.h"
#include "Runtime/Runtime.h"
#include "Runtime/Kernel.h"
#include "Runtime/Input.h"
#include "Runtime/Video.h"

int main()
{
	runtime_init();

	// rt_video_set_mode(VMODE_320_200_8);

	// rt_video_set_palette(0, 0x000000);
	// rt_video_set_palette(1, 0xffffff);

	// uint8_t* p = rt_video_get_primary_target();

	for (;;)
	{
		rt_event_t e;
		while (rt_input_get_event(&e))
		{
			printf("%d %d ", e.x, e.y);

			// p[e.x + e.y * 320] = 1;

			if (e.button != 0)
			{
				switch (e.button)
				{
					case RT_INPUT_BUTTON_A:
						printf("A");
						break;
					case RT_INPUT_BUTTON_B:
						printf("B");
						break;
					case RT_INPUT_BUTTON_C:
						printf("C");
						break;
					case RT_INPUT_BUTTON_D:
						printf("D");
						break;
					case RT_INPUT_BUTTON_S1:
						printf("S1");
						break;
					case RT_INPUT_BUTTON_S2:
						printf("S2");
						break;
					case RT_INPUT_DPAD_N:
						printf("DPAD N");
						break;
					case RT_INPUT_DPAD_S:
						printf("DPAD S");
						break;
					case RT_INPUT_DPAD_E:
						printf("DPAD E");
						break;
					case RT_INPUT_DPAD_W:
						printf("DPAD W");
						break;
					case RT_INPUT_TB:
						printf("TB");
						break;
				}

				if (e.pressed)
					printf(" pressed");
				else
					printf(" released");
			}
			printf("\n");
		}
	}

	return 0;
}
