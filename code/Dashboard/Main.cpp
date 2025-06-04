#include <stdio.h>
#include <stdlib.h>

// #include <HAL/Audio.h>
// #include <HAL/Video.h>
#include <HAL/UART.h>

#include "Runtime/CRT.h"
#include "Runtime/File.h"
#include "Runtime/Runtime.h"
#include "Runtime/Kernel.h"


int main()
{
    //runtime_init();
    crt_init();

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

    for (;;)
    {
    	printf("Hello world from Dashboard!\n");
        // hal_uart_tx_u8('A');
        // hal_uart_tx_u8('B');
        // hal_uart_tx_u8(13);
        // hal_uart_tx_u8(10);
    }


    return 0;
}
