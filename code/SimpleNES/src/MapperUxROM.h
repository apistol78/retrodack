#pragma once

#include "Mapper.h"

namespace sn
{

class MapperUxROM final : public Mapper
{
public:
    MapperUxROM(Cartridge& cart);

    virtual Byte readPRG(Address addr) const override;
    
    virtual void writePRG(Address addr, Byte value) override;

    virtual Byte readCHR(Address addr) const override;

    virtual void writeCHR(Address addr, Byte value) override;

private:
    bool              m_usesCharacterRAM;
    const Byte*       m_lastBankPtr;
    Address           m_selectPRG;
    std::vector<Byte> m_characterRAM;
};

}
