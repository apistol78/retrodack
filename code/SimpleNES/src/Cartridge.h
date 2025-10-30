#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sn
{

using Byte = std::uint8_t;
using Address = std::uint16_t;

class Cartridge
{
public:
    Cartridge() = default;
    
    bool loadFromFile(const std::string& path);

    const std::vector<Byte>& getROM() const { return m_PRG_ROM; };

    const std::vector<Byte>& getVROM() const { return m_CHR_ROM; };

    Byte getMapper() const { return m_mapperNumber; };

    Byte getNameTableMirroring() const { return m_nameTableMirroring; };

    bool hasExtendedRAM() const 
    { 
        // Some ROMs don't have this set correctly, plus there's no particular reason to disable it.
        return true; 
    };

private:
    std::vector<Byte> m_PRG_ROM;
    std::vector<Byte> m_CHR_ROM;
    Byte m_nameTableMirroring = 0;
    Byte m_mapperNumber = 0;
    bool m_extendedRAM = false;
    bool m_chrRAM = false;
};

};
