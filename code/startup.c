
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

void delay()
{
    for (int i = 0; i < 1000000; ++i)
    {
        __asm__ volatile (
            "nop\n"
            :
            :
        );
    }
}

void _start()
{
    // Setup stack.
	const uint32_t sp = 0x10000000 + 0x00001000;
	__asm__ volatile (
		"mv sp, %0	\n"
		:
		: "r" (sp)
	);


    volatile uint32_t* pin = (uint32_t*)0x40000000;

    volatile uint32_t* sdram = (uint32_t*)0x20000000;

    for (uint32_t i = 0; i < 100000; ++i)
    {
        sdram[i] = i;
    }

    int status = 0;
    for (uint32_t i = 0; i < 100000; ++i)
    {
        if (sdram[i] != i)
            status = 1;
    }


    for (;;)
    {
        *pin = status ? 1 : 2;
        delay();
        *pin = 0;
        delay();
    }



/*
    volatile uint32_t* uart = (uint32_t*)0x30000000;
    for (;;)
    {
        *uart = 'H';
        *uart = 'E';
        *uart = 'L';
        *uart = 'L';
        *uart = 'O';
        *uart = ' ';
        *uart = 'W';
        *uart = 'O';
        *uart = 'R';
        *uart = 'L';
        *uart = 'D';
        *uart = ' ';
    }
*/
}
