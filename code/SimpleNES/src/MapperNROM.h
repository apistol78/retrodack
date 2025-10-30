#pragma once

#include "Mapper.h"

namespace sn
{

class MapperNROM final : public Mapper
{
public:
    MapperNROM(Cartridge& cart);

    virtual Byte readPRG(Address addr) const override;
    
    virtual void writePRG(Address addr, Byte value) override;

    virtual Byte readCHR(Address addr) const override;

    virtual void writeCHR(Address addr, Byte value) override;

private:
    bool m_oneBank;
    bool m_usesCharacterRAM;
    // std::vector<Byte> m_characterRAM;
};

}
