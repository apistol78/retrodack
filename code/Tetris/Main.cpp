#include <stdio.h>

#include "Runtime/Runtime.h"


extern int tetrisMain();

int main()
{
	runtime_init();

	rt_video_set_mode(VMODE_360_360_8);

	rt_video_set_palette(0, 0x808080);
  	rt_video_set_palette(1, 0xff0000);
  	rt_video_set_palette(2, 0x00ff00);
  	rt_video_set_palette(3, 0x0000ff);
  	rt_video_set_palette(4, 0xffff00);
  	rt_video_set_palette(5, 0xff00ff);
  	rt_video_set_palette(6, 0x00ffff);
  	rt_video_set_palette(7, 0xffffff);
	rt_video_set_palette(8, 0x000000);

	rt_video_clear(0);
	rt_video_wait();

	tetrisMain();

	return 0;
}
