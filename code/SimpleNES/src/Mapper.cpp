#include "Mapper.h"
#include "MapperAxROM.h"
#include "MapperCNROM.h"
#include "MapperColorDreams.h"
#include "MapperGxROM.h"
#include "MapperMMC3.h"
#include "MapperNROM.h"
#include "MapperSxROM.h"
#include "MapperUxROM.h"

namespace sn
{

Mapper::Mapper(Cartridge& cart, Type t)
: m_cartridge(cart)
, m_type(t)
{
    if (cart.getVROM().size() == 0)
    {
        m_characterRAM.resize(0x2000);
    }
    else
    {
        m_characterRAM = m_cartridge.getVROM();
    }
}

NameTableMirroring Mapper::getNameTableMirroring() const
{
    return static_cast<NameTableMirroring>(m_cartridge.getNameTableMirroring());
}

std::unique_ptr<Mapper> Mapper::createMapper(Mapper::Type              mapper_t,
                                             sn::Cartridge&            cart,
                                             IRQHandle&                irq,
                                             std::function<void(void)> mirroring_cb)
{
    std::unique_ptr<Mapper> ret(nullptr);
    switch (mapper_t)
    {
    case NROM:
        ret.reset(new MapperNROM(cart));
        break;
    case SxROM:
        ret.reset(new MapperSxROM(cart, mirroring_cb));
        break;
    case UxROM:
        ret.reset(new MapperUxROM(cart));
        break;
    case CNROM:
        ret.reset(new MapperCNROM(cart));
        break;
    case MMC3:
        ret.reset(new MapperMMC3(cart, irq, mirroring_cb));
        break;
    case AxROM:
        ret.reset(new MapperAxROM(cart, mirroring_cb));
        break;
    case ColorDreams:
        ret.reset(new MapperColorDreams(cart, mirroring_cb));
        break;
    case GxROM:
        ret.reset(new MapperGxROM(cart, mirroring_cb));
        break;
    default:
        break;
    }
    return ret;
}
}
