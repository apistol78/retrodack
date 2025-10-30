#pragma once

#include <functional>
#include "PictureBus.h"
#include "VirtualScreen.h"

namespace sn
{

constexpr int ScanlineCycleLength = 341;
constexpr int ScanlineEndCycle    = 340;
constexpr int VisibleScanlines    = 240;
constexpr int ScanlineVisibleDots = 256;
constexpr int FrameEndScanline    = 261;
constexpr int AttributeOffset     = 0x3C0;

class PPU
{
public:
    explicit PPU(PictureBus& bus, VirtualScreen& screen);
    
    void reset();
    
    void step(int nsteps);

    void setInterruptCallback(std::function<void(void)> cb);

    void doDMA(const Byte* page_ptr);

    // Callbacks mapped to CPU address space
    // Addresses written to by the program
    void control(Byte ctrl);

    void setMask(Byte mask);

    void setOAMAddress(Byte addr) { m_spriteDataAddress = addr;}

    void setDataAddress(Byte addr);

    void setScroll(Byte scroll);

    void setData(Byte data);

    // Read by the program
    Byte getStatus();
    
    Byte getData();

    Byte getOAMData() const { return readOAM(m_spriteDataAddress); }

    void setOAMData(Byte value) { writeOAM(m_spriteDataAddress++, value);}

private:
    PictureBus& m_bus;
    VirtualScreen& m_screen;
    std::function<void(void)> m_vblankCallback;
    Byte m_spriteMemory[64 * 4];
    Byte m_scanlineSprites[64];
    Byte m_scanlineSpritesNum = 0;

    Byte readOAM(Byte addr) const { return m_spriteMemory[addr]; }

    void writeOAM(Byte addr, Byte value) { m_spriteMemory[addr] = value; }

    enum State
    {
        PreRender,
        Render,
        PostRender,
        VerticalBlank
    } m_pipelineState;

    int m_cycle;
    int m_scanline;
    int m_scanlineOffset;
    bool m_evenFrame;

    bool m_vblank;
    bool m_sprZeroHit;
    bool m_spriteOverflow;

    // Registers
    Address m_dataAddress;
    Address m_tempAddress;
    Byte m_fineXScroll;
    bool m_firstWrite;
    Byte m_dataBuffer;

    Byte m_spriteDataAddress;

    // Setup flags and variables
    bool m_longSprites;
    bool m_generateInterrupt;

    bool m_greyscaleMode;
    bool m_showSprites;
    bool m_showBackground;
    bool m_hideEdgeSprites;
    bool m_hideEdgeBackground;

    enum CharacterPage
    {
        Low,
        High,
    } m_bgPage,
      m_sprPage;

    Address m_dataAddrIncrement;
};

}
