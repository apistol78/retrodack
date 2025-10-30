#pragma once

#include "Mapper.h"

namespace sn
{
class MapperCNROM final : public Mapper
{
public:
    MapperCNROM(Cartridge& cart);

    virtual Byte readPRG(Address addr) const override;

    virtual void writePRG(Address addr, Byte value) override;

    virtual Byte readCHR(Address addr) const override;

    virtual void writeCHR(Address addr, Byte value) override;

private:
    bool m_oneBank;
    Address m_selectCHR;
};

}
