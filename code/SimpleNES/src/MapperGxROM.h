#include "Mapper.h"

namespace sn
{

class MapperGxROM final : public Mapper
{
public:
    MapperGxROM(Cartridge& cart, std::function<void(void)> mirroring_cb);

    virtual NameTableMirroring getNameTableMirroring() const override;

    virtual Byte readPRG(Address address) const override;
    
    virtual void writePRG(Address address, Byte value) override;

    virtual Byte readCHR(Address address) const override;

    virtual void writeCHR(Address address, Byte value) override;

    Byte               prgbank;
    Byte               chrbank;

private:
    NameTableMirroring        m_mirroring;

    std::vector<Byte>         m_characterRAM;
    std::function<void(void)> m_mirroringCallback;
};

}
