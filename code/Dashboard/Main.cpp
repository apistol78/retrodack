#include <stdio.h>
#include <stdlib.h>

#include <HAL/Audio.h>
#include <HAL/Video.h>

#include "Runtime/File.h"
#include "Runtime/Runtime.h"
#include "Runtime/Kernel.h"


void __register_exitproc(void) {}
void __call_exitprocs(void) {}

int main()
{
    runtime_init();
    
    // kernel_cs_init(&lock);
    // kernel_create_thread(&thread_1);
    // kernel_create_thread(&thread_2);

    // for (int i = 0; i < 256; ++i)
    // {
    //     hal_video_set_palette(
    //         i,
    //         (rand() & 255) | ((rand() & 255) << 8) | ((rand() & 255) << 16)
    //     );
    // }
	
	// hal_video_present();

	printf("Hello world from Dashboard!\n");

    for (;;)
    {
    }


    return 0;
}