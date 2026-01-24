#include <Core/Log/Log.h>
#include <Core/Misc/String.h>
#include <Core/Thread/ThreadManager.h>

#include <Emulator2/VCDTrace.h>
#include <Emulator2/CPU/Bus.h>
#include <Emulator2/CPU/GDBServer.h>
#include <Emulator2/CPU/Helpers.h>
#include <Emulator2/CPU/GL/CPU_gate.h>
#include <Emulator2/CPU/HL/CPU_hl.h>
#include <Emulator2/Devices/Audio.h>
#include <Emulator2/Devices/DMA.h>
#include <Emulator2/Devices/I2C.h>
#include <Emulator2/Devices/Memory.h>
#include <Emulator2/Devices/PLIC.h>
#include <Emulator2/Devices/SD.h>
#include <Emulator2/Devices/SPI.h>
#include <Emulator2/Devices/Sprite.h>
#include <Emulator2/Devices/Timer.h>
#include <Emulator2/Devices/UART.h>
#include <Emulator2/Devices/Video.h>
#include <Emulator2/Devices/Unknown.h>

#include "Emulator/Emulator.h"
#include "Emulator/FileSystemImage.h"
#include "Emulator/FuelGauge.h"
#include "Emulator/GPIOExtender.h"
#include "Emulator/LoadELF.h"
#include "Emulator/LoadHEX.h"
#include "Emulator/Profiler.h"
#include "Emulator/RTC.h"
#include "Emulator/SignalView.h"
#include "Emulator/TrackBallDevice.h"

using namespace traktor;

namespace
{

template < typename T >
T snoopReadData(const ICPU* cpu, uint32_t addr)
{
	constexpr uint32_t ns = (sizeof(T) + 3) / 4;
	uint32_t d[ns];
	for (uint32_t i = 0; i < ns; ++i)
		d[i] = cpu->snoopReadU32(addr + i * 4);
	return *(const T*)d;
}

}

T_IMPLEMENT_RTTI_CLASS(L"Emulator", Emulator, Object)

bool Emulator::create(FileSystemImage* fs, bool highLevelCPU, bool traceFST, bool traceVCD)
{
    m_fs = fs;

    // Create bus devices.
	m_rom = new Memory(0x00100000, false);
 	m_sdram = new Memory(0x02000000, true);
	m_video = new Video(720, 720);
	m_uart = new UART();
	m_i2c = new I2C();
	m_sd = new SD(m_fs->ptr(), m_fs->size());
	m_timer = new ::Timer();
	m_plic = new PLIC();
	m_audio = new Audio();
	m_dma = new DMA();
	m_sprite = new Sprite();
	m_spi = new SPI();

    // Create I2C devices.
    m_trackBallDevice = new TrackBallDevice();
    m_gpioExtender = new GPIOExtender();
    m_fuelGauge = new FuelGauge();
    m_rtc = new RTC();

    // Add I2C devices to I2C bus device.
    m_i2c->addSlave(0x0a, m_trackBallDevice);
    m_i2c->addSlave(0x20, m_gpioExtender);
    m_i2c->addSlave(0x55, m_fuelGauge);
    m_i2c->addSlave(0x68, m_rtc);

    // Expose sprite device to video.
    m_video->setSprite(m_sprite);

    // Setup the bus.
    m_bus = new Bus();
	m_bus->map(0x00000000, 0x00000000 + m_rom->getCapacity(), m_rom);
 	m_bus->map(0x10000000, 0x10000000 + m_sdram->getCapacity(), m_sdram);
	m_bus->map(0x20000000, 0x20000100, m_uart);
	m_bus->map(0x30000000, 0x30000100, m_i2c);
	m_bus->map(0x40000000, 0x40000100, m_sd);
	m_bus->map(0x50000000, 0x50000100, m_timer);
	m_bus->map(0x60000000, 0x60ffffff, m_audio);
	m_bus->map(0x70000000, 0x70ffffff, m_plic);
	m_bus->map(0x80000000, 0x81000000, m_video);
	m_bus->map(0x90000000, 0x90000100, m_dma);
	m_bus->map(0xa0000000, 0xa0010000, m_sprite);
	m_bus->map(0xb0000000, 0xb0010000, m_spi);

    // Create the CPU.
    if (highLevelCPU)
	{
		log::info << L"[EMU] using high level CPU emulation." << Endl;
		m_cpu = new CPU_hl(m_bus, nullptr, true);
	}
	else
	{
		log::info << L"[EMU] using gate level CPU emulation." << Endl;
		m_cpu = new CPU_gate(m_bus, traceFST ? "CPU_gate.fst" : nullptr);
	}
    m_cpu->setSP(0x12000000 - 4);

	// Create GDB server.
	m_gdbServer = new GDBServer(m_cpu, m_bus);
	m_gdbServer->create();

    // Create CPU profiler.
    m_profiler = new Profiler();

    // Create VCD recorder.
	if (traceVCD)
	{
		m_vcd->declare(L"TIMER");
		m_vcd->declare(L"INPUT");
		m_vcd->declare(L"GPIO");
		m_vcd->declare(L"VIDEO");
		m_vcd->declare(L"AUDIO");
		m_vcd->declare(L"COUNTDOWN", [&, this]() {
            return m_timer->getCountDown() > 0;
        });
		m_vcd->declare(L"TIP", [&, this]() {
			const uint32_t mip = m_cpu->getCSR(MIP);
			return (mip & 0x80) != 0;
		});
	}

	// Setup PLIC interrupts.
	m_timer->setCallback([&, this]() {
        if (m_enableInterrupt)
        {
            if (m_vcd) 
                m_vcd->toggle(0);
            //m_cpu->interrupt(TIMER);
			m_cpu->getInterruptPending() |= TIMER;
			m_irqCounters[0]++;
        }
    });
	m_trackBallDevice->setCallback([&, this]() {
        if (m_enableInterrupt)
        {
            if (m_vcd)
                m_vcd->toggle(1);
            m_plic->raise(0);
			m_irqCounters[1]++;
        }
    });
	m_gpioExtender->setCallback([&, this]() {
        if (m_enableInterrupt)
        {
            if (m_vcd)
                m_vcd->toggle(2);
            m_plic->raise(0);
			m_irqCounters[1]++;
        }
    });
	m_video->setCallback([&, this]() {
        if (m_enableInterrupt)
        {
            if (m_vcd) 
                m_vcd->toggle(3);
            m_plic->raise(2);
			m_irqCounters[2]++;
        }
    });
	m_audio->setCallback([&, this]() {
        if (m_enableInterrupt)
        {
            if (m_vcd)
                m_vcd->toggle(4);
            m_plic->raise(1);
			m_irqCounters[3]++;
        }
    });
    
	// CPU execution thread.
	m_threadCpu = ThreadManager::getInstance().create([&, this]()
	{
		traktor::Timer timer;
		while(!m_threadCpu->stopped())
		{
			if (m_vcd)
				m_vcd->tick();

			m_gdbServer->process();
			switch (m_gdbServer->getMode())
			{
			case GDBServer::ModeRun:
				{
					for (int32_t i = 0; i < 10000; ++i)
					{
						if (!m_cpu->tick(1) || m_bus->error())
						{
							m_gdbServer->setMode(GDBServer::ModeStopped);
							break;
						}

						m_gdbServer->tick();
						if (m_gdbServer->getMode() != GDBServer::ModeRun)
							break;

						readDebugVector();
					}
				}
				break;

			case GDBServer::ModeStep:
				{
					const uint32_t fromPC = m_cpu->getPC();
					m_enableInterrupt = false;
					while (m_cpu->getPC() == fromPC)
					{
						if (!m_cpu->tick(1) || m_bus->error())
							break;
					}
					m_enableInterrupt = true;
					m_gdbServer->setMode(GDBServer::ModeStopped);
				}
				break;

			case GDBServer::ModeStopped:
			case GDBServer::ModeKilled:
			default:
				m_threadCpu->sleep(0);
				break;
			}

			if (timer.getElapsedTime() > 10.0f / 1000.0f)
			{
				m_profiler->record(m_cpu->getPC());
				timer.reset();
			}
		}
	});
	m_threadCpu->start(); 
    return true;  
}



	// if (vcd)
	// {
	// 	Ref< IStream > fs = FileSystem::getInstance().open(L"Emulator.vcd", File::FmWrite);
	// 	FileOutputStream fos(fs, new Utf8Encoding());
	// 	vcd->dump(fos);
	// 	fos.close();
	// }

bool Emulator::loadELF(const std::wstring& fileName)
{
    return ::loadELF(fileName, *m_cpu, *m_bus);
}

void Emulator::actionContinue()
{
    m_gdbServer->setMode(GDBServer::ModeRun);
}

void Emulator::actionPause()
{
    m_gdbServer->setMode(GDBServer::ModeStopped);
}

void Emulator::inputMovement(int32_t dx, int32_t dy)
{
    m_trackBallDevice->accumulateMovement(dx, dy);
}

void Emulator::inputButton(bool b)
{
    m_trackBallDevice->setButton(b);
}

void Emulator::inputSetBit(int32_t index, bool b)
{
    m_gpioExtender->setInputBit(index, b);
}

void Emulator::shutdown()
{
	m_threadCpu->stop();
	ThreadManager::getInstance().destroy(m_threadCpu);
	m_threadCpu = nullptr;
}

bool Emulator::alive() const
{
    return m_threadCpu != nullptr && !m_threadCpu->wait(0);
}

void Emulator::readDebugVector()
{
	struct debug_thread_t
	{
		uint32_t id;
		uint32_t name_addr;
		uint32_t stack_addr;
		uint32_t sp;
		uint32_t epc;
		uint32_t sleep;
		uint32_t waiting_addr;
	};

	struct debug_vector_t
	{
		uint32_t threads_addr;
		uint32_t current_addr;
		uint32_t count_addr;	
	};

	const uint32_t dva = m_cpu->getCSR(MSCRATCH);
	if (dva == 0)
		return;

	const debug_vector_t dv = snoopReadData< debug_vector_t >(m_cpu, dva);
	
	const uint32_t count = m_cpu->snoopReadU32(dv.count_addr);
	for (uint32_t i = 0; i < count; ++i)
	{
		const debug_thread_t dt = snoopReadData< debug_thread_t >(m_cpu, dv.threads_addr + i * sizeof(debug_thread_t));
		if (dt.waiting_addr != 0)
			m_threadWaitingCounters[i]++;
		else if (dt.sleep != 0)
			m_threadSleepingCounters[i]++;
	}

	const uint32_t current = m_cpu->snoopReadU32(dv.current_addr);
	if (current < 16)
	 	m_threadActiveCounters[current]++;

	
}