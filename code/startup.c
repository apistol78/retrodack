
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

void _start()
{
    // Setup stack.
	const uint32_t sp = 0x10000000 + 0x00001000;
	__asm__ volatile (
		"mv sp, %0	\n"
		:
		: "r" (sp)
	);

    // 
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
}
