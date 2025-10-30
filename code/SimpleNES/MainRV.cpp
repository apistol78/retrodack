#include <Runtime/Runtime.h>

#include "Emulator.h"

int main(int argc, char** argv)
{
	runtime_init();

    {
        sn::Emulator emulator;
        emulator.run("zelda.nes");
    }

    return 0;
}
