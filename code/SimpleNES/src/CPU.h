#pragma once

#include <list>
#include "CPUOpcodes.h"
#include "MainBus.h"

namespace sn
{

class CPU;

class IRQHandle
{
    int  bit;
    CPU& cpu;

public:
    IRQHandle(int bit, CPU& cpu)
      : bit(bit)
      , cpu(cpu)
    {
    }

    void release();

    void pull();
};

class CPU
{

public:
    explicit CPU(MainBus& mem);

    void step();

    void reset();

    void reset(Address start_addr);

    void log();

    Address getPC() const { return r_PC; }

    void skipOAMDMACycles();

    void skipDMCDMACycles();

    void nmiInterrupt();

    IRQHandle& createIRQHandler();

    void setIRQPulldown(int bit, bool state);

private:
    void interruptSequence(InterruptType type);

    // Instructions are split into five sets to make decoding easier.
    // These functions return true if they succeed
    bool executeImplied(Byte opcode);

    bool executeBranch(Byte opcode);

    bool executeType0(Byte opcode);

    bool executeType1(Byte opcode);

    bool executeType2(Byte opcode);

    Address readAddress(Address addr) const { return Address(m_bus.read(addr)) | (Address(m_bus.read(addr + 1)) << 8); }

    void pushStack(Byte value) { m_bus.write(0x100 | r_SP, value); --r_SP; }

    Byte pullStack() { return m_bus.read(0x100 | ++r_SP); }

    // If a and b are in different pages, increases the m_SkipCycles by 1
    void skipPageCrossCycle(Address a, Address b);
    
    void setZN(Byte value);

    int                   m_skipCycles;
    int                   m_cycles;

    // Registers
    Address               r_PC;
    Byte                  r_SP;
    Byte                  r_A;
    Byte                  r_X;
    Byte                  r_Y;

    // Status flags.
    // Is storing them in one byte better ?
    bool                  f_C;
    bool                  f_Z;
    bool                  f_I;
    bool                  f_D;
    bool                  f_V;
    bool                  f_N;

    bool                  m_pendingNMI;

    bool                  isPendingIRQ() const { return !f_I && m_irqPulldowns != 0; };

    MainBus&              m_bus;

    // Each bit is assigned to an IRQ handler.
    // If any bits are set, it means the irq must be triggered
    int                   m_irqPulldowns = 0;
    std::list<IRQHandle> m_irqHandlers;
};

}
