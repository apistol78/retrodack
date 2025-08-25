#if defined(__LINUX__) || defined(__RPI__) || defined(__APPLE__)
#	include <sys/types.h>
#	include <sys/stat.h>
#	include <signal.h>
#	include <stdio.h>
#	include <stdlib.h>
#	include <fcntl.h>
#	include <errno.h>
#	include <unistd.h>
#	include <syslog.h>
#	include <string.h>
#endif

#include <cmath>
#include <cstring>

// Traktor
#include <Core/Io/FileOutputStream.h>
#include <Core/Io/FileSystem.h>
#include <Core/Io/MemoryStream.h>
#include <Core/Io/Utf8Encoding.h>
#include <Core/Log/Log.h>
#include <Core/Misc/CommandLine.h>
#include <Core/Misc/String.h>
#include <Core/Misc/TString.h>
#include <Core/Timer/Timer.h>
#include <Core/Thread/ThreadManager.h>
#include <Core/Thread/Thread.h>
#include <Drawing/Image.h>
#include <Ui/Application.h>
#include <Ui/AspectLayout.h>
#include <Ui/Bitmap.h>
#include <Ui/Image.h>
#include <Ui/Form.h>
#include <Ui/FloodLayout.h>
#if defined(_WIN32)
#	include <Ui/Win32/WidgetFactoryWin32.h>
#elif defined(__APPLE__)
#	include <Ui/Cocoa/WidgetFactoryCocoa.h>
#elif defined(__LINUX__) || defined(__RPI__)
#	include <Ui/X11/WidgetFactoryX11.h>
#endif

// Klara-RV
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
#include <Emulator2/Devices/Timer.h>
#include <Emulator2/Devices/UART.h>
#include <Emulator2/Devices/Video.h>
#include <Emulator2/Devices/Unknown.h>

//
#include "Emulator/FileSystemImage.h"
#include "Emulator/GPIOExtender.h"
#include "Emulator/LoadELF.h"
#include "Emulator/LoadHEX.h"
#include "Emulator/TrackBallDevice.h"

using namespace traktor;

bool g_going = true;

#if defined(__LINUX__) || defined(__RPI__) || defined(__APPLE__)
void abortHandler(int s)
{
	g_going = false;
}
#endif

void dumpCallStack(const ICPU* cpu, const Bus* bus, OutputStream& os)
{
	uint32_t fp = cpu->getRegister(8);
	uint32_t sp = cpu->getRegister(2);
	// uint32_t pc = cpu->getPC();

	os << L"Call stack (FP " << str(L"%08x", fp) << L"):" << Endl;

	for (int i = 0; i < 8; ++i)
	{
		uint32_t f_ra = bus->readU32(fp - 8);
		uint32_t f_fp = bus->readU32(fp - 4);

		os << L"   " << i << L": FP "  << str(L"%08x", f_fp) << L", RA " << str(L"%08x", f_ra) << Endl;

		fp = f_fp;
	}

	/*
	int max_depth = 16;
	int depth = 0;

	while (fp && depth < max_depth)
	{
		uint32_t ra = bus->readU32(fp - 4);
		if (ra == 0)
			break;

		os << L"   " << depth << L": " << str(L"%08x", ra) << Endl;

		fp = bus->readU32(fp - 8);
		depth++;
	}
	*/	
}

int main(int argc, const char** argv)
{
	const CommandLine cmdLine(argc, argv);

#if defined(__LINUX__) || defined(__RPI__) || defined(__APPLE__)
	{
		struct sigaction sa = { SIG_IGN };
		sigaction(SIGPIPE, &sa, nullptr);
	}
	{
		struct sigaction sa;
		sa.sa_handler = abortHandler;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sigaction(SIGINT, &sa, nullptr);
	}
#endif

#if defined(_WIN32)
	ui::Application::getInstance()->initialize(
		new ui::WidgetFactoryWin32(),
		nullptr
	);
#elif defined(__APPLE__)
	ui::Application::getInstance()->initialize(
		new ui::WidgetFactoryCocoa(),
		nullptr
	);
#elif defined(__LINUX__) || defined(__RPI__)
	ui::Application::getInstance()->initialize(
		new ui::WidgetFactoryX11(),
		nullptr
	);
#endif

	// Create file system from files.
	Ref< FileSystemImage > fsi = FileSystemImage::createFromDirectory(L"fs");
	if (!fsi)
	{
		log::error << L"Unable to create in-memory file system!" << Endl;
		return 1;
	}

	// Create emulation devices.
	Memory rom(0x00100000);
 	Memory sdram(0x02000000);
	Video video(720, 720);
	UART uart;
	I2C i2c;
	SD sd(fsi->ptr(), fsi->size());
	::Timer tmr;
	PLIC plic;
	Audio audio;
	DMA dma;

	TrackBallDevice tb;
	i2c.addSlave(0x0a, &tb);

	GPIOExtender gpio;
	i2c.addSlave(0x20, &gpio);

	Bus bus;
	bus.map(0x00000000, 0x00000000 + rom.getCapacity(), false, false, &rom);
 	bus.map(0x10000000, 0x10000000 + sdram.getCapacity(), true, false, &sdram);
	bus.map(0x20000000, 0x20000100, false, false, &uart);
	bus.map(0x30000000, 0x30000100, false, true, &i2c);
	bus.map(0x40000000, 0x40000100, false, true, &sd);
	bus.map(0x50000000, 0x50000100, false, true, &tmr);
	bus.map(0x60000000, 0x60000100, false, true, &audio);
	bus.map(0x70000000, 0x70ffffff, false, true, &plic);
	bus.map(0x80000000, 0x81000000, false, false, &video);
	bus.map(0x90000000, 0x90000100, false, true, &dma);

	Ref< OutputStream > os = nullptr;	
	if (cmdLine.hasOption(L't', L"trace"))
	{
		Ref< traktor::IStream > f = FileSystem::getInstance().open(L"CPU.trace", File::FmWrite);
		if (f)
			os = new FileOutputStream(f, new Utf8Encoding());
	}

	Ref< ICPU > cpu;
	std::wstring trace;

	if (cmdLine.hasOption(L"hl"))
	{
		log::info << L"Using high level CPU emulation." << Endl;
		cpu = new CPU_hl(&bus, nullptr, true);
		// trace = L"RD_h.trace";
	}
	else
	{
		log::info << L"Using gate level CPU emulation." << Endl;

		if (cmdLine.hasOption(L"fst"))
			cpu = new CPU_gate(&bus, "CPU_gate.fst");
		else
			cpu = new CPU_gate(&bus, nullptr);

		// trace = L"RD_g.trace";
	}

	cpu->setSP(0x12000000 - 4);

	VCDTrace vcd;
	vcd.declare(L"TIMER");

	tmr.setCallback([&](){ vcd.set(0, true); cpu->interrupt(TIMER); });
	tb.setCallback([&](){ plic.raise(0); }); // Input interrupt
	gpio.setCallback([&](){ plic.raise(1); }); // GPIO interrupt
	audio.setCallback([&]() { plic.raise(2); }); // Audio interrupt

	if (cmdLine.hasOption(L'e', L"elf"))
	{
		const std::wstring fileName = cmdLine.getOption(L'e', L"elf").getString();
		if (!loadELF(fileName, *cpu, bus))
			return 1;
		if (bus.error())
		{
			log::error << L"BUS error after loading ELF." << Endl;
			return 1;
		}
	}
	
	if (cmdLine.hasOption(L'h', L"hex"))
	{
		sdram.setReadOnly(true);

		const std::wstring fileName = cmdLine.getOption(L'h', L"hex").getString();
		if (!loadHEX(fileName, *cpu, bus))
			return 1;
		if (bus.error())
		{
			log::error << L"BUS error after loading HEX." << Endl;
			return 1;
		}

		sdram.setReadOnly(false);
	}

	// Create GDB server.
	Ref< GDBServer > gdbs = new GDBServer(cpu, &bus);
	gdbs->create();

	// Do not set read-only if a GDB server is attached.
	// rom.setReadOnly(true);


	// Create user interface.
	Ref< ui::Form > form = new ui::Form();
	form->create(L"RetroDACK", 720_ut, 720_ut, ui::Form::WsDefault, new ui::FloodLayout());

	Ref< ui::Bitmap > uiImage = new ui::Bitmap(720, 720);

	Ref< ui::Container > container = new ui::Container();
	container->create(form, ui::WsNone, new ui::AspectLayout());
	
	Ref< ui::Image > image = new ui::Image();
	image->create(container, uiImage, ui::Image::WsScale | ui::Image::WsNearestFilter);

	form->update();
	form->show();


	ui::Point mousePosition(0, 0);

	image->addEventHandler< ui::MouseButtonDownEvent >([&](ui::MouseButtonDownEvent* event) {
		if (event->getButton() == ui::MbtLeft)
			tb.setButton(true);
	});
	image->addEventHandler< ui::MouseButtonUpEvent >([&](ui::MouseButtonUpEvent* event) {
		if (event->getButton() == ui::MbtLeft)
			tb.setButton(false);
	});
	image->addEventHandler< ui::MouseMoveEvent >([&](ui::MouseMoveEvent* event) {
		const auto d = event->getPosition() - mousePosition;
		mousePosition = event->getPosition();
		tb.accumulateMovement(d.cx, d.cy);
	});
	image->addEventHandler< ui::KeyDownEvent >([&](ui::KeyDownEvent* event) {
		switch (event->getVirtualKey())
		{
		case ui::Vk1:
			gpio.setInputBit(0, true);
			break;
		case ui::Vk2:
			gpio.setInputBit(1, true);
			break;
		case ui::Vk3:
			gpio.setInputBit(2, true);
			break;
		case ui::Vk4:
			gpio.setInputBit(3, true);
			break;
		case ui::Vk5:
			gpio.setInputBit(4, true);
			break;
		case ui::Vk6:
			gpio.setInputBit(5, true);
			break;
		case ui::VkW:
			gpio.setInputBit(6, true);
			break;
		case ui::VkS:
			gpio.setInputBit(7, true);
			break;
		case ui::VkD:
			gpio.setInputBit(8, true);
			break;
		case ui::VkA:
			gpio.setInputBit(9, true);
			break;
		}
	});
	image->addEventHandler< ui::KeyUpEvent >([&](ui::KeyUpEvent* event){
		if (event->isRepeat())
			return;

		switch (event->getVirtualKey())
		{
		case ui::Vk1:
			gpio.setInputBit(0, false);
			break;
		case ui::Vk2:
			gpio.setInputBit(1, false);
			break;
		case ui::Vk3:
			gpio.setInputBit(2, false);
			break;
		case ui::Vk4:
			gpio.setInputBit(3, false);
			break;
		case ui::Vk5:
			gpio.setInputBit(4, false);
			break;
		case ui::Vk6:
			gpio.setInputBit(5, false);
			break;
		case ui::VkW:
			gpio.setInputBit(6, false);
			break;
		case ui::VkS:
			gpio.setInputBit(7, false);
			break;
		case ui::VkD:
			gpio.setInputBit(8, false);
			break;
		case ui::VkA:
			gpio.setInputBit(9, false);
			break;
		}
	});

	if (!trace.empty())
	{
		Ref< IStream > f = FileSystem::getInstance().open(trace, File::FmWrite);
		if (f)
			os = new FileOutputStream(f, new Utf8Encoding()); 
	}

	Thread* th = ThreadManager::getInstance().create([&]()
	{
		uint32_t mode = 2;
		uint32_t pc = ~0U;

		while(!th->stopped())
		{
			// vcd.tick();
			// vcd.set(0, false);

			if (gdbs)
				gdbs->process(mode);

			switch (mode)
			{
			case 0:	// Run
				{
					if (!cpu->tick(1) || bus.error())
					{
						pc = cpu->getPC();
						mode = 2;
					}
				}
				break;

			case 1:	// Step
				{
					if (!cpu->tick(1) || bus.error())
					{
						pc = cpu->getPC();
						mode = 2;
					}
					if (pc != cpu->getPC())
					{
						pc = cpu->getPC();
						mode = 2;
					}
					th->sleep(0);
				}
				break;

			case 2:	// Stopped
			case 3:	// Killed
				th->sleep(0);
				break;
			}

			// if (!cpu->tick(ticksPerIteration) || bus.error())
			// {
			// 	cpu->flushCaches();

			// 	log::error << L"CPU tick failed at PC " << str(L"%08x", cpu->getPC()) << Endl;

			// 	log::info << str(L"%-5S", L"PC") << L" : " << str(L"%08x", cpu->getPC()) << Endl;
			// 	log::info << L"---" << Endl;

			// 	for (uint32_t i = 0; i < 32; ++i)
			// 		log::info << str(L"%-5S", getRegisterName(i)) << L" : " << str(L"%08x", cpu->getRegister(i)) << Endl;

			// 	dumpCallStack(cpu, &bus, log::info);

			// 	break;
			// }

			// if (os)
			// {
			// 	(*os) << str(L"%08x", cpu->getPC());
			// 	for (uint32_t i = 0; i < 32; ++i)
			// 		(*os) << L":" << str(L"%08x", cpu->getRegister(i));
			// 	(*os) << Endl;
			// }
		}
	});
	th->start();

	traktor::Timer timer;
	while (g_going && !th->wait(0))
	{
		if (!ui::Application::getInstance()->process())
			break;

		if (timer.getElapsedTime() > 1.0f / 60.0f)
		{
			drawing::Image* videoImage = video.getImage();
			if (videoImage)
			{
				if (uiImage)
				{
					const ui::Size sz = uiImage->getSize(form);
					if (sz.cx != videoImage->getWidth() || sz.cy != videoImage->getHeight())
					{
						uiImage->destroy();
						uiImage->create(videoImage);
					}
					else
						uiImage->copyImage(videoImage);
				}
				image->setImage(uiImage);
			}
			timer.reset();
		}			
	}

	th->stop();
	ThreadManager::getInstance().destroy(th);

	if (form)
	{
		form->destroy();
		form = nullptr;
	}

	/*
	Ref< IStream > fs = FileSystem::getInstance().open(L"Emulator.vcd", File::FmWrite);
	FileOutputStream fos(fs, new Utf8Encoding());
	vcd.dump(fos);
	fos.close();
	*/

	return 0;
}
