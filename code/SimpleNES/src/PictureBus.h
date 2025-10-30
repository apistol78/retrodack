#pragma once

#include "Cartridge.h"
#include "Mapper.h"

namespace sn
{

class PictureBus
{
public:
    PictureBus();

    Byte read_2000_2fff(Address addr) const
    {
        const Address index = addr & 0x3ff;

        if (NameTable0 >= sizeof(m_RAM))
            return m_mapper->readCHR(addr);
        else if (addr < 0x2400) // NT0
            return m_RAM[NameTable0 + index];
        else if (addr < 0x2800) // NT1
            return m_RAM[NameTable1 + index];
        else if (addr < 0x2c00) // NT2
            return m_RAM[NameTable2 + index];
        else /* if (addr < 0x3000)*/ // NT3
            return m_RAM[NameTable3 + index];    
    }        

    Byte read(Address addr) const;

    void write(Address addr, Byte value);

    bool setMapper(Mapper* mapper);

    Byte readPalette(Byte paletteAddr) const 
    {
        // Addresses $3F10/$3F14/$3F18/$3F1C are mirrors of $3F00/$3F04/$3F08/$3F0C
        if (paletteAddr >= 0x10 && (paletteAddr & 3) == 0)
        {
            paletteAddr = paletteAddr & 0xf;
        }
        return m_palette[paletteAddr];       
    }

    void updateMirroring();

    void scanlineIRQ();

private:
    std::size_t NameTable0, NameTable1, NameTable2, NameTable3; // indices where they start in RAM vector
    Byte m_palette[0x20];
    Byte m_RAM[0x800];
    Mapper* m_mapper;

    using readCHR_fn = Byte(*)(Mapper*, Address);

    readCHR_fn m_mapperReadCHR;
};

}
