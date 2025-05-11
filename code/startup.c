
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

void _start()
{
    volatile uint32_t* uart = (uint32_t*)0x30000000;
    for (;;)
    {
        *uart = 'H';
        *uart = 'E';
        *uart = 'L';
        *uart = 'L';
        *uart = 'O';
    }
}
