#include <stdio.h>
#include <stdlib.h>

#include <hal/Video.h>

#include "Runtime/Runtime.h"
#include "Runtime/Kernel.h"

static volatile kernel_cs_t lock;

void thread_1()
{
    for (;;)
    {
        kernel_cs_lock(&lock);
        printf("thread 1\n");
        kernel_cs_unlock(&lock);
        kernel_sleep(500);
    }
}

void thread_2()
{
    for (;;)
    {
        kernel_cs_lock(&lock);
        printf("thread 2\n");
        kernel_cs_unlock(&lock);
        kernel_sleep(500);
    }
}


int main()
{
    runtime_init();
    printf("Hello world!\n");
    
    kernel_cs_init(&lock);
    kernel_create_thread(&thread_1);
    kernel_create_thread(&thread_2);

    for (int i = 0; i < 256; ++i)
    {
        video_set_palette(
            i,
            (rand() & 255) | ((rand() & 255) << 8) | ((rand() & 255) << 16)
        );
    }

    for (int j = 0; j < 10; ++j)
    {
        printf("%d\n", j);
        volatile uint8_t* vmem = (volatile uint8_t*)video_get_primary_target();
        for (uint32_t i = 0; i < 720 * 720; ++i)
            vmem[i] = rand();
        video_present();
    }

    for (;;)
    {
        printf("main\n");
        kernel_sleep(1000);
    }

    return 0;
}