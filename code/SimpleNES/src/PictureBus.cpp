#include "Mapper.h"
#include "PictureBus.h"

namespace sn
{

PictureBus::PictureBus()
  : m_mapper(nullptr)
{
}

Byte PictureBus::read(Address addr) const
{
    // PictureBus is limited to 0x3fff
    addr = addr & 0x3fff;

    if (addr < 0x2000)
    {
        // return m_mapper->readCHR(addr);
        return m_mapper->readCHR_novptr(addr);
    }
    else if (addr <= 0x3eff)
    {
        const Address index = addr & 0x3ff;

        // Name tables upto 0x3000, then mirrored upto 3eff
        Address normalizedAddr = addr;
        if (addr >= 0x3000)
        {
            normalizedAddr -= 0x1000;
        }

        if (NameTable0 >= sizeof(m_RAM))
            // return m_mapper->readCHR(normalizedAddr);
            return m_mapper->readCHR_novptr(normalizedAddr);
        else if (normalizedAddr < 0x2400) // NT0
            return m_RAM[NameTable0 + index];
        else if (normalizedAddr < 0x2800) // NT1
            return m_RAM[NameTable1 + index];
        else if (normalizedAddr < 0x2c00) // NT2
            return m_RAM[NameTable2 + index];
        else /* if (normalizedAddr < 0x3000)*/ // NT3
            return m_RAM[NameTable3 + index];
    }
    else if (addr <= 0x3fff)
    {
        const Address paletteAddr = addr & 0x1f;
        return readPalette(paletteAddr);
    }

    return 0;
}

void PictureBus::write(Address addr, Byte value)
{
    // PictureBus is limited to 0x3fff
    addr = addr & 0x3fff;

    if (addr < 0x2000)
    {
        // m_mapper->writeCHR(addr, value);
        m_mapper->writeCHR_novptr(addr, value);
    }
    else if (addr <= 0x3eff)
    {
        const auto index          = addr & 0x3ff;
        // Name tables upto 0x3000, then mirrored upto 3eff
        auto       normalizedAddr = addr;
        if (addr >= 0x3000)
        {
            normalizedAddr -= 0x1000;
        }

        if (NameTable0 >= sizeof(m_RAM))
            m_mapper->writeCHR(normalizedAddr, value);
        else if (normalizedAddr < 0x2400) // NT0
            m_RAM[NameTable0 + index] = value;
        else if (normalizedAddr < 0x2800) // NT1
            m_RAM[NameTable1 + index] = value;
        else if (normalizedAddr < 0x2c00) // NT2
            m_RAM[NameTable2 + index] = value;
        else // NT3
            m_RAM[NameTable3 + index] = value;
    }
    else if (addr <= 0x3fff)
    {
        auto palette = addr & 0x1f;
        // Addresses $3F10/$3F14/$3F18/$3F1C are mirrors of $3F00/$3F04/$3F08/$3F0C
        if (palette >= 0x10 && addr % 4 == 0)
        {
            palette = palette & 0xf;
        }
        m_palette[palette] = value;
    }
}

void PictureBus::updateMirroring()
{
    switch (m_mapper->getNameTableMirroring())
    {
    case Horizontal:
        NameTable0 = NameTable1 = 0;
        NameTable2 = NameTable3 = 0x400;
        // LOG(InfoVerbose) << "Horizontal Name Table mirroring set. (Vertical Scrolling)" << std::endl;
        break;

    case Vertical:
        NameTable0 = NameTable2 = 0;
        NameTable1 = NameTable3 = 0x400;
        // LOG(InfoVerbose) << "Vertical Name Table mirroring set. (Horizontal Scrolling)" << std::endl;
        break;

    case OneScreenLower:
        NameTable0 = NameTable1 = NameTable2 = NameTable3 = 0;
        // LOG(InfoVerbose) << "Single Screen mirroring set with lower bank." << std::endl;
        break;

    case OneScreenHigher:
        NameTable0 = NameTable1 = NameTable2 = NameTable3 = 0x400;
        // LOG(InfoVerbose) << "Single Screen mirroring set with higher bank." << std::endl;
        break;

    case FourScreen:
        NameTable0 = sizeof(m_RAM);
        // LOG(InfoVerbose) << "FourScreen mirroring." << std::endl;
        break;

    default:
        NameTable0 = NameTable1 = NameTable2 = NameTable3 = 0;
        // LOG(Error) << "Unsupported Name Table mirroring : " << m_mapper->getNameTableMirroring() << std::endl;
    }
}

bool PictureBus::setMapper(Mapper* mapper)
{
    if (!mapper)
    {
        // LOG(Error) << "Mapper argument is nullptr" << std::endl;
        return false;
    }

    m_mapper = mapper;
    updateMirroring();
    return true;
}

void PictureBus::scanlineIRQ()
{
    m_mapper->scanlineIRQ();
}

}
