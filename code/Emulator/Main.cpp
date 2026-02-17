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
#include <Ui/Application.h>
#include <Ui/Bitmap.h>
#include <Ui/Image.h>
#include <Ui/StyleBitmap.h>
#include <Ui/StyleSheet.h>

#if defined(_WIN32)
#	include <Ui/Win32/WidgetFactoryWin32.h>
#elif defined(__APPLE__)
#	include <Ui/Cocoa/WidgetFactoryCocoa.h>
#elif defined(__LINUX__) || defined(__RPI__)
#	include <Ui/X11/WidgetFactoryX11.h>
#endif

//
#include "Emulator/Emulator.h"
#include "Emulator/FileSystemImage.h"
#include "Emulator/MainForm.h"

using namespace traktor;

bool g_going = true;

#if defined(__LINUX__) || defined(__RPI__) || defined(__APPLE__)
void abortHandler(int s)
{
	g_going = false;
}
#endif

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

	// Create emulator.
	Ref< Emulator > emulator = new Emulator();
	if (!emulator->create(
		fsi,
		cmdLine.hasOption(L"hl"),
		cmdLine.hasOption(L"fst"),
		cmdLine.hasOption(L"vcd")
	))
	{
		log::error << L"Unable to create emulator." << Endl;
		return 1;
	}

	// Load binary.
	if (cmdLine.hasOption(L'e', L"elf"))
	{
		const std::wstring fileName = cmdLine.getOption(L'e', L"elf").getString();
		if (!emulator->loadELF(fileName))
		{
			log::error << L"Unable to load ELF." << Endl;
			return 1;
		}
	}

	// Create user interface.
	Ref< MainForm > mainForm = new MainForm(emulator);
	if (!mainForm->create())
	{
		log::error << L"Unable to create user interface." << Endl;
		return 1;
	}
	
	if (cmdLine.hasOption('r', L"run"))
		emulator->actionContinue();

	traktor::Timer timer;
	double lastVideoT = 0.0;
	while (g_going && emulator->alive())
	{
		if (!ui::Application::getInstance()->process())
			break;

		if ((timer.getElapsedTime() - lastVideoT) > 1.0f / 60.0f)
		{
			mainForm->updateVideo();
			mainForm->update();
			lastVideoT = timer.getElapsedTime();
		}
	}

	if (mainForm)
	{
		mainForm->destroy();
		mainForm = nullptr;
	}

	ui::Application::getInstance()->finalize();
	return 0;
}
