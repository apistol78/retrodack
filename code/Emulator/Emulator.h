#pragma once

#include <Core/Object.h>
#include <Core/Ref.h>
#include <Core/Thread/Thread.h>

class Audio;
class Bus;
class DMA;
class FileSystemImage;
class FuelGauge;
class GDBServer;
class GPIOExtender;
class I2C;
class ICPU;
class Memory;
class PLIC;
class Profiler;
class RTC;
class SD;
class SPI;
class Sprite;
class Timer;
class TrackBallDevice;
class UART;
class VCDTrace;
class Video;

class Emulator : public traktor::Object
{
    T_RTTI_CLASS;

public:
    bool create(FileSystemImage* fs, bool highLevelCPU, bool traceFST, bool traceVCD);

    bool loadELF(const std::wstring& fileName);

    void actionContinue();

    void actionPause();

    void inputMovement(int32_t dx, int32_t dy);

    void inputButton(bool b);

    void inputSetBit(int32_t index, bool b);

    void shutdown();

    bool alive() const;

    Video* getVideo() const { return m_video; }

private:
    traktor::Ref< FileSystemImage > m_fs;

    // Bus devices.
    traktor::Ref< Memory > m_rom;
    traktor::Ref< Memory > m_sdram;
    traktor::Ref< Video > m_video;
    traktor::Ref< UART > m_uart;
    traktor::Ref< I2C > m_i2c;
    traktor::Ref< SD > m_sd;
    traktor::Ref< ::Timer > m_timer;
    traktor::Ref< PLIC > m_plic;
    traktor::Ref< Audio > m_audio;
    traktor::Ref< DMA > m_dma;
    traktor::Ref< Sprite > m_sprite;
    traktor::Ref< SPI > m_spi;

    // I2C devices.
    traktor::Ref< TrackBallDevice > m_trackBallDevice;
    traktor::Ref< GPIOExtender > m_gpioExtender;
    traktor::Ref< FuelGauge > m_fuelGauge;
    traktor::Ref< RTC > m_rtc;

    // CPU and BUS.
    traktor::Ref< Bus > m_bus;
    traktor::Ref< ICPU > m_cpu;

    traktor::Ref< Profiler > m_profiler;
    traktor::Ref< VCDTrace > m_vcd;
    traktor::Ref< GDBServer > m_gdbServer;
    traktor::Thread* m_threadCpu = nullptr;
    bool m_enableInterrupt = true;
};
