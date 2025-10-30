#pragma once

#include "Mapper.h"
#include "PictureBus.h"

namespace sn
{

class MapperAxROM final : public Mapper
{
public:
    MapperAxROM(Cartridge& cart, std::function<void(void)> mirroring_cb);

    virtual Byte readPRG(Address address) const override;

    virtual void writePRG(Address address, Byte value) override;

    virtual Byte readCHR(Address address) const override;

    virtual void writeCHR(Address address, Byte value) override;

    virtual NameTableMirroring getNameTableMirroring() const override;

private:
    NameTableMirroring        m_mirroring;
    std::function<void(void)> m_mirroringCallback;
    uint32_t                  m_prgBank;
    std::vector<Byte>         m_characterRAM;
};

}
