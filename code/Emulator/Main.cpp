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
#include <Core/System/OS.h>
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
#include <Ui/TableLayout.h>
#include <Ui/StyleBitmap.h>
#include <Ui/StyleSheet.h>
#include <Ui/StatusBar/StatusBar.h>
#include <Ui/ToolBar/ToolBar.h>
#include <Ui/ToolBar/ToolBarButton.h>
#include <Ui/ToolBar/ToolBarButtonClickEvent.h>

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
#include <Emulator2/Devices/SPI.h>
#include <Emulator2/Devices/Sprite.h>
#include <Emulator2/Devices/Timer.h>
#include <Emulator2/Devices/UART.h>
#include <Emulator2/Devices/Video.h>
#include <Emulator2/Devices/Unknown.h>

//
#include "Emulator/FileSystemImage.h"
#include "Emulator/GPIOExtender.h"
#include "Emulator/LoadELF.h"
#include "Emulator/LoadHEX.h"
#include "Emulator/Profiler.h"
#include "Emulator/SignalView.h"
#include "Emulator/TrackBallDevice.h"

#define MSTATUS_MIE_BIT_MASK	0x8
#define MSTATUS_MPIE_BIT_MASK	0x80
#define MIE_MTI_BIT_MASK		0x80

using namespace traktor;

bool g_going = true;

#if defined(__LINUX__) || defined(__RPI__) || defined(__APPLE__)
void abortHandler(int s)
{
	g_going = false;
}
#endif

void dumpMemory(const Bus* bus)
{
	Ref< IStream > fs = FileSystem::getInstance().open(L"memory.bin", File::FmWrite);
	for (uint32_t addr = 0x10000000; addr < 0x12000000; addr += 4)
	{
		const uint32_t data = bus->readU32(addr);
		fs->write(&data, 4);
	}
	fs->close();
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

	// Check if environment is already set, else set to current working directory.
	std::wstring home;
	if (!OS::getInstance().getEnvironment(L"RETRODACK_HOME", home))
	{
		const Path executablePath = OS::getInstance().getExecutable().getPathOnly();
		FileSystem::getInstance().setCurrentVolumeAndDirectory(executablePath);

		while (!FileSystem::getInstance().exist(L"LICENSE.txt"))
		{
			const Path cwd = FileSystem::getInstance().getCurrentVolumeAndDirectory();
			const Path pwd = cwd.getPathOnly();
			if (cwd == pwd)
			{
				log::error << L"No LICENSE.txt file found." << Endl;
				return 1;
			}
			FileSystem::getInstance().setCurrentVolumeAndDirectory(pwd);
		}

		const Path cwd = FileSystem::getInstance().getCurrentVolumeAndDirectory();
		OS::getInstance().setEnvironment(L"RETRODACK_HOME", cwd.getPathNameOS());
	}

	Ref< const ui::StyleSheet > styleSheet = ui::StyleSheet::load(L"$(RETRODACK_HOME)/resources/themes/Shared/StyleSheet.xss");
	if (!styleSheet)
	{
		log::error << L"Unable to load stylesheet." << Endl;
		return 1;
	}

	// Append our styles into current.
	ui::Application::getInstance()->appendStyleSheet(styleSheet);

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
	DMA dma0_0;
	DMA dma0_1;
	DMA dma0_2;
	DMA dma1_0;
	DMA dma1_1;
	DMA dma1_2;
	Sprite sprite;
	SPI spi;

	TrackBallDevice tb;
	i2c.addSlave(0x0a, &tb);

	GPIOExtender gpio;
	i2c.addSlave(0x20, &gpio);

	video.setSprite(&sprite);

	Bus bus;
	bus.map(0x00000000, 0x00000000 + rom.getCapacity(), false, false, &rom);
 	bus.map(0x10000000, 0x10000000 + sdram.getCapacity(), true, false, &sdram);
	bus.map(0x20000000, 0x20000100, false, false, &uart);
	bus.map(0x30000000, 0x30000100, false, true, &i2c);
	bus.map(0x40000000, 0x40000100, false, true, &sd);
	bus.map(0x50000000, 0x50000100, false, true, &tmr);
	bus.map(0x60000000, 0x60000100, false, false, &audio);
	bus.map(0x70000000, 0x70ffffff, false, true, &plic);
	bus.map(0x80000000, 0x81000000, false, true, &video);
	bus.map(0x90000000, 0x90000100, false, true, &dma0_0);
	bus.map(0x91000000, 0x91000100, false, true, &dma0_1);
	bus.map(0x92000000, 0x92000100, false, true, &dma0_2);
	bus.map(0xa0000000, 0xa0000100, false, true, &dma1_0);
	bus.map(0xa1000000, 0xa1000100, false, true, &dma1_1);
	bus.map(0xa2000000, 0xa2000100, false, true, &dma1_2);
	bus.map(0xb0000000, 0xb0010000, false, true, &sprite);
	bus.map(0xc0000000, 0xc0010000, false, false, &spi);

	Ref< OutputStream > os;

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

	// Create GDB server.
	Ref< GDBServer > gdbServer = new GDBServer(cpu, &bus);
	gdbServer->create();

	cpu->setSP(0x12000000 - 4);

	// Create VCD trace.
	Ref< VCDTrace > vcd;
	if (cmdLine.hasOption(L"vcd"))
	{
		vcd = new VCDTrace();
		vcd->declare(L"TIMER");
		vcd->declare(L"INPUT");
		vcd->declare(L"GPIO");
		vcd->declare(L"DMA");
		vcd->declare(L"VIDEO");
		vcd->declare(L"COUNTDOWN", [&]() { return tmr.getCountDown() > 0; });
		vcd->declare(L"TIP", [&]() {
			const uint32_t mip = cpu->getCSR(MIP);
			return (mip & 0x80) != 0;
		});
	}

	bool g_enableInterrupt = true;

	// Setup PLIC interrupts.
	tmr.setCallback([&](){ if (g_enableInterrupt) { if (vcd) { vcd->toggle(0); } cpu->interrupt(TIMER); } } );
	tb.setCallback([&](){ if (g_enableInterrupt) { if (vcd) { vcd->toggle(1); } plic.raise(0); } });		// Input interrupt
	gpio.setCallback([&](){ if (g_enableInterrupt) { if (vcd) { vcd->toggle(2); } plic.raise(0); } });		// GPIO interrupt
	// dma0.setCallback([&]() { if (g_enableInterrupt) { if (vcd) { vcd->toggle(3); } plic.raise(1); } });		// DMA interrupt
	// dma1.setCallback([&]() { if (g_enableInterrupt) { if (vcd) { vcd->toggle(3); } plic.raise(1); } });		// DMA interrupt
	// dma2.setCallback([&]() { if (g_enableInterrupt) { if (vcd) { vcd->toggle(3); } plic.raise(1); } });		// DMA interrupt
	video.setCallback([&]() { if (g_enableInterrupt) { if (vcd) { vcd->toggle(4); } plic.raise(2); } });	// Video interrupt
	// usb.setCallback([&]() { plic.raise(3); }); // USB interrupt

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

	// Do not set read-only if a GDB server is attached.
	if (!gdbServer)
		rom.setReadOnly(true);

	// Create user interface.
	Ref< ui::Form > form = new ui::Form();
	form->create(L"RetroDACK", 220_ut, 220_ut, ui::Form::WsDefault, new ui::TableLayout(L"100%", L"*,100%,*", 0_ut, 0_ut));
	form->addEventHandler< ui::CloseEvent >([&](ui::CloseEvent* event) {
		g_going = false;
		event->consume();
	});

	Ref< ui::ToolBar > toolBar = new ui::ToolBar();
	toolBar->create(form, ui::WsNone);
	toolBar->addImage(new ui::StyleBitmap(L"Emulator.Play"));
	toolBar->addItem(new ui::ToolBarButton(L"Continue", 0, ui::Command(L"Emulator.Continue")));
	toolBar->addItem(new ui::ToolBarButton(L"Pause", 0, ui::Command(L"Emulator.Pause")));
	toolBar->addEventHandler< ui::ToolBarButtonClickEvent >([&](ui::ToolBarButtonClickEvent* event)
	{
		const std::wstring cmd = event->getCommand().getName();
		if (cmd == L"Emulator.Continue")
		{
			gdbServer->setMode(GDBServer::ModeRun);
		}
		else if (cmd == L"Emulator.Pause")
		{
			gdbServer->setMode(GDBServer::ModeStopped);
		}
	});

	Ref< ui::Bitmap > uiImage = new ui::Bitmap(720, 720);

	Ref< ui::Container > container = new ui::Container();
	container->create(form, ui::WsNone, new ui::AspectLayout());
	
	Ref< ui::Image > image = new ui::Image();
	image->create(container, uiImage, ui::Image::WsScale | ui::Image::WsNearestFilter);

	Ref< ui::Container > containerThreads = new ui::Container();
	containerThreads->create(form, ui::WsNone, new ui::TableLayout(L"100%", L"*", 0_ut, 0_ut));
	
	Ref< SignalView > signalThreads[6];
	for (int i = 0; i < 6; ++i)
	{
		signalThreads[i] = new SignalView();
		signalThreads[i]->create(containerThreads, 0);
	}

	form->update();
	form->show();

	float lptx = 0.0f, lpty = 0.0f;
	float dptx = 0.0f, dpty = 0.0f;

	image->addEventHandler< ui::MouseButtonDownEvent >([&](ui::MouseButtonDownEvent* event) {
		if (event->getButton() == ui::MbtLeft)
			tb.setButton(true);
	});
	image->addEventHandler< ui::MouseButtonUpEvent >([&](ui::MouseButtonUpEvent* event) {
		if (event->getButton() == ui::MbtLeft)
			tb.setButton(false);
	});
	image->addEventHandler< ui::MouseMoveEvent >([&](ui::MouseMoveEvent* event) {

		const ui::Size sz = image->getInnerRect().getSize();
		const ui::Point pt = event->getPosition();
		
		float fptx = (float)pt.x / (sz.cx / 180.0f);
		float fpty = (float)pt.y / (sz.cy / 180.0f);

		dptx += fptx - lptx;
		dpty += fpty - lpty;

		lptx = fptx;
		lpty = fpty;

		int32_t idptx = (int32_t)dptx;
		int32_t idpty = (int32_t)dpty;

		dptx -= idptx;
		dpty -= idpty;

		tb.accumulateMovement(idptx, idpty);
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

	Ref< Profiler > profiler = new Profiler();

	// CPU execution thread.
	Thread* threadCpu = ThreadManager::getInstance().create([&]()
	{
		traktor::Timer timer;
		while(!threadCpu->stopped())
		{
			if (vcd)
				vcd->tick();

			gdbServer->process();
			switch (gdbServer->getMode())
			{
			case GDBServer::ModeRun:
				{
					for (int32_t i = 0; i < 1000; ++i)
					{
						if (!cpu->tick(1) || bus.error())
						{
							gdbServer->setMode(GDBServer::ModeStopped);
							break;
						}

						gdbServer->tick();
						if (gdbServer->getMode() != GDBServer::ModeRun)
							break;
					}
				}
				break;

			case GDBServer::ModeStep:
				{
					const uint32_t fromPC = cpu->getPC();
					g_enableInterrupt = false;
					while (cpu->getPC() == fromPC)
					{
						if (!cpu->tick(1) || bus.error())
							break;
					}
					g_enableInterrupt = true;
					gdbServer->setMode(GDBServer::ModeStopped);
				}
				break;

			case GDBServer::ModeStopped:
			case GDBServer::ModeKilled:
			default:
				threadCpu->sleep(0);
				break;
			}

			const int32_t ct = cpu->getCSR(MSCRATCH) & 0xffff;
			for (int i = 0; i < 6; ++i)
			{
				signalThreads[i]->set(0, (i == ct) ? 1 : 0);
			}

			if (timer.getElapsedTime() > 10.0f / 1000.0f)
			{
				profiler->record(cpu->getPC());
				timer.reset();
			}
		}
	});
	threadCpu->start();

	if (cmdLine.hasOption('r', L"run"))
		gdbServer->setMode(GDBServer::ModeRun);

	traktor::Timer timer;
	double lastVideoT = 0.0;

	while (g_going && !threadCpu->wait(0))
	{
		if (!ui::Application::getInstance()->process())
			break;

		if (
			(timer.getElapsedTime() - lastVideoT) > 1.0f / 60.0f
		)
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
			lastVideoT = timer.getElapsedTime();

			for (int i = 0; i < 6; ++i)
			{
				signalThreads[i]->update();
			}
		}
	}

	threadCpu->stop();
	ThreadManager::getInstance().destroy(threadCpu);

	if (form)
	{
		form->destroy();
		form = nullptr;
	}

	profiler = nullptr;

	if (vcd)
	{
		Ref< IStream > fs = FileSystem::getInstance().open(L"Emulator.vcd", File::FmWrite);
		FileOutputStream fos(fs, new Utf8Encoding());
		vcd->dump(fos);
		fos.close();
	}

	return 0;
}
