#pragma once

#include "Mapper.h"

namespace sn
{

class MapperSxROM final : public Mapper
{
public:
    MapperSxROM(Cartridge& cart, std::function<void(void)> mirroring_cb);

    virtual Byte readPRG(Address addr) const override;

    virtual void writePRG(Address addr, Byte value) override;

    virtual Byte readCHR(Address addr) const override;

    virtual void writeCHR(Address addr, Byte value) override;

    NameTableMirroring getNameTableMirroring();

private:
    void                      calculatePRGPointers();

    std::function<void(void)> m_mirroringCallback;
    NameTableMirroring        m_mirroing;

    bool                      m_usesCharacterRAM;
    int                       m_modeCHR;
    int                       m_modePRG;

    Byte                      m_tempRegister;
    int                       m_writeCounter;

    Byte                      m_regPRG;
    Byte                      m_regCHR0;
    Byte                      m_regCHR1;

    const Byte*               m_firstBankPRG;
    const Byte*               m_secondBankPRG;

    int                       m_firstBankCHRIdx;
    int                       m_secondBankCHRIdx;

    std::vector<Byte>         m_characterRAM;
};

}
