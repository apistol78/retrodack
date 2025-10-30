#include "MapperColorDreams.h"

namespace sn
{

MapperColorDreams::MapperColorDreams(Cartridge& cart, std::function<void(void)> mirroring_cb)
  : Mapper(cart, Mapper::ColorDreams)
  , m_mirroring(Vertical)
  , m_mirroringCallback(mirroring_cb)
{
}

Byte MapperColorDreams::readPRG(Address address) const
{
    if (address >= 0x8000)
    {
        return m_cartridge.getROM()[(prgbank * 0x8000) + (address & 0x7fff)];
    }
    return 0;
}

void MapperColorDreams::writePRG(Address address, Byte value)
{
    if (address >= 0x8000)
    {
        prgbank = ((value >> 0) & 0x3);
        chrbank = ((value >> 4) & 0xF);
        m_characterRAMOffset = chrbank * 0x2000;
    }
}

Byte MapperColorDreams::readCHR(Address address) const
{
    if (address <= 0x1FFF)
    {
        return m_cartridge.getVROM()[(chrbank * 0x2000) + address];
    }
    return 0;
}

NameTableMirroring MapperColorDreams::getNameTableMirroring() const
{
    return m_mirroring;
}

void MapperColorDreams::writeCHR(Address, Byte)
{
}

}
