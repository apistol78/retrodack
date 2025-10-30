#pragma optimize

#include "Mapper.h"

namespace sn
{

class MapperColorDreams final : public Mapper
{
public:
    MapperColorDreams(Cartridge& cart, std::function<void(void)> mirroring_cb);

    virtual NameTableMirroring getNameTableMirroring() const override;

    virtual Byte readPRG(Address address) const override;

    virtual void writePRG(Address address, Byte value) override;

    virtual Byte readCHR(Address address) const override;

    virtual void writeCHR(Address address, Byte value) override;

private:
    NameTableMirroring        m_mirroring;
    uint32_t                  prgbank;
    uint32_t                  chrbank;
    std::function<void(void)> m_mirroringCallback;
};

}
