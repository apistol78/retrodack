#include <stdio.h>
#include "doomgeneric.h"
#include "doomkeys.h"

#include <Runtime/Runtime.h>
#include <Runtime/Timer.h>
#include <Runtime/Video.h>

void DG_Init()
{
	runtime_init();
	rt_video_set_mode(VMODE_320_200_8);
}

void DG_DrawFrame()
{
}

void DG_DrawFrame2(const uint8_t* frame, const uint32_t* colors)
{
	static int count = 0;
	if (++count >= 60)
	{
		static uint32_t last_ms = 0;
		uint32_t ms = rt_timer_get_ms();
		printf("%d fps\n", (60 * 1000) / (ms - last_ms));
		last_ms = ms;
		count = 0;
	}

	for (uint32_t i = 0; i < 256; ++i)
		rt_video_set_palette(i, colors[i]);

	rt_video_blit(frame);
	rt_video_present();
}

void DG_SleepMs(uint32_t ms)
{
	rt_timer_wait_ms(ms);
}

uint32_t DG_GetTicksMs()
{
	return rt_timer_get_ms();
}

int DG_GetKey(int* pressed, unsigned char* doomKey)
{
	// uint8_t ikc;
	// uint8_t im;
	// uint8_t ip;

	// if (input_get_kb_event(&ikc, &im, &ip))
	// {
	// 	switch (ikc)
	// 	{
	// 	case RT_KEY_LEFT:
	// 		*doomKey = KEY_LEFTARROW;
	// 		break;

	// 	case RT_KEY_A:
	// 		*doomKey = KEY_STRAFE_L;
	// 		break;

	// 	case RT_KEY_RIGHT:
	// 		*doomKey = KEY_RIGHTARROW;
	// 		break;

	// 	case RT_KEY_D:
	// 		*doomKey = KEY_STRAFE_R;
	// 		break;

	// 	case RT_KEY_UP:
	// 	case RT_KEY_W:
	// 		*doomKey = KEY_UPARROW;
	// 		break;

	// 	case RT_KEY_DOWN:
	// 	case RT_KEY_S:
	// 		*doomKey = KEY_DOWNARROW;
	// 		break;

	// 	case RT_KEY_E:
	// 		*doomKey = KEY_USE;
	// 		break;

	// 	case RT_KEY_SPACE:
	// 		*doomKey = KEY_FIRE;
	// 		break;

	// 	case RT_KEY_ESCAPE:
	// 		*doomKey = KEY_ESCAPE;
	// 		break;

	// 	case RT_KEY_RETURN:
	// 	case RT_KEY_ENTER:
	// 		*doomKey = KEY_ENTER;
	// 		break;
	// 	}

	// 	*pressed = ip;
	// 	return 1;
	// }
	
	return 0;
}

void DG_SetWindowTitle(const char* title)
{
}
