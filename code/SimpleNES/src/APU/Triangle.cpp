#include "APU/Triangle.h"
#include "APU/Divider.h"
#include "APU/Units.h"
#include "Cartridge.h"
// #include <SFML/Audio/SoundStream.hpp>
// #include <SFML/Config.hpp>
// #include <SFML/System/Time.hpp>

namespace sn
{

void Triangle::set_period(int p)
{
    period = p;
    sequencer.set_period(p);
}

// Clocked at the cpu freq
void Triangle::clock()
{
    if (length_counter.muted())
    {
        return;
    }

    if (linear_counter.counter == 0)
    {
        return;
    }

    if (sequencer.clock())
    {
        seq_idx = (seq_idx + 1) % 32;
    }
}

Byte Triangle::sample() const
{
    return volume();
}

int Triangle::volume() const
{
    if (seq_idx < 16)
    {
        return 15 - seq_idx;
    }
    else
    {
        return seq_idx - 16;
    }
}

}
