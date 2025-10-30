#pragma once

#include <functional>
#include <memory>
#include "Cartridge.h"
#include "CPU.h"

namespace sn
{

class IRQHandle;

enum NameTableMirroring
{
    Horizontal = 0,
    Vertical   = 1,
    FourScreen = 8,
    OneScreenLower,
    OneScreenHigher
};

class Mapper
{
public:
    enum Type
    {
        NROM        = 0,
        SxROM       = 1,
        UxROM       = 2,
        CNROM       = 3,
        MMC3        = 4,
        AxROM       = 7,
        ColorDreams = 11,
        GxROM       = 66,
    };

    explicit Mapper(Cartridge& cart, Type t)
      : m_cartridge(cart)
      , m_type(t)
    {}

    virtual ~Mapper() = default;

    virtual Byte readPRG(Address addr) const = 0;

    virtual void writePRG(Address addr, Byte value) = 0;

    virtual Byte readCHR(Address addr) const = 0;

    virtual void writeCHR(Address addr, Byte value) = 0;

    virtual NameTableMirroring getNameTableMirroring();

    bool inline hasExtendedRAM() { return m_cartridge.hasExtendedRAM(); }

    virtual void scanlineIRQ() {}

    static std::unique_ptr<Mapper> createMapper(Type mapper_t, Cartridge& cart, IRQHandle& irq, std::function<void(void)> mirroring_cb);


    Byte readCHR_novptr(Address addr) const { return m_characterRAM[addr]; }

    void writeCHR_novptr(Address addr, Byte value) { m_characterRAM[addr] = value; }


protected:
    Cartridge& m_cartridge;
    Type m_type;

    std::vector<Byte> m_characterRAM;
};

}
