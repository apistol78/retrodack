#include <Core/Misc/String.h>
#include <Drawing/Image.h>
#include <Ui/Application.h>
#include <Ui/AspectLayout.h>
#include <Ui/Form.h>
#include <Ui/FloodLayout.h>
#include <Ui/GridView/GridColumn.h>
#include <Ui/GridView/GridItem.h>
#include <Ui/GridView/GridRow.h>
#include <Ui/MultiSplitter.h>
#include <Ui/TableLayout.h>
#include <Ui/Splitter.h>
#include <Ui/StyleBitmap.h>
#include <Ui/StyleSheet.h>
#include <Ui/StatusBar/StatusBar.h>
#include <Ui/ToolBar/ToolBar.h>
#include <Ui/ToolBar/ToolBarButton.h>
#include <Ui/ToolBar/ToolBarButtonClickEvent.h>

#include <Emulator2/CPU/Helpers.h>
#include <Emulator2/CPU/ICPU.h>
#include <Emulator2/Devices/Video.h>

#include "Emulator/Emulator.h"
#include "Emulator/MainForm.h"

using namespace traktor;

T_IMPLEMENT_RTTI_CLASS(L"MainForm", MainForm, ui::Form)

MainForm::MainForm(Emulator* emulator)
:   m_emulator(emulator)
{
}

bool MainForm::create()
{
	if (!ui::Form::create(L"RetroDACK", 720_ut, 420_ut, ui::Form::WsDefault, new ui::TableLayout(L"100%", L"*,100%", 0_ut, 0_ut)))
        return false;

	addEventHandler< ui::CloseEvent >([&](ui::CloseEvent* event) {
		m_emulator->shutdown();
		event->consume();
	});

	Ref< ui::ToolBar > toolBar = new ui::ToolBar();
	toolBar->create(this, ui::WsNone);
	toolBar->addImage(new ui::StyleBitmap(L"Emulator.Play"));
	toolBar->addImage(new ui::StyleBitmap(L"Emulator.Stop"));
	toolBar->addItem(new ui::ToolBarButton(L"Continue", 0, ui::Command(L"Emulator.Continue")));
	toolBar->addItem(new ui::ToolBarButton(L"Pause", 1, ui::Command(L"Emulator.Pause")));
	toolBar->addEventHandler< ui::ToolBarButtonClickEvent >([&](ui::ToolBarButtonClickEvent* event) {
		const std::wstring& cmd = event->getCommand().getName();
		if (cmd == L"Emulator.Continue")
            m_emulator->actionContinue();
		else if (cmd == L"Emulator.Pause")
            m_emulator->actionPause();
	});

	Ref< ui::Splitter > splitter = new ui::Splitter();
	splitter->create(this, true, -300_ut);

	Ref< ui::Container > containerImage = new ui::Container();
	containerImage->create(splitter, ui::WsNone, new ui::AspectLayout());
	
	m_uiImage = new ui::Bitmap(720, 720);
	m_image = new ui::Image();
	m_image->create(containerImage, m_uiImage, ui::Image::WsScale | ui::Image::WsNearestFilter);
	m_image->addEventHandler< ui::MouseButtonDownEvent >(this, &MainForm::imageMouseButtonDown);
	m_image->addEventHandler< ui::MouseButtonUpEvent >(this, &MainForm::imageMouseButtonUp);
	m_image->addEventHandler< ui::MouseMoveEvent >(this, &MainForm::imageMouseMove);
	m_image->addEventHandler< ui::KeyDownEvent >(this, &MainForm::imageKeyDown);
	m_image->addEventHandler< ui::KeyUpEvent >(this, &MainForm::imageKeyUp);

	Ref< ui::MultiSplitter > splitter2 = new ui::MultiSplitter();
	splitter2->create(splitter, false);

	m_gridRegisters = new ui::GridView();
	m_gridRegisters->create(splitter2, ui::GridView::WsColumnHeader);
	m_gridRegisters->addColumn(new ui::GridColumn(L"Register", 60_ut));
	m_gridRegisters->addColumn(new ui::GridColumn(L"Value", 80_ut));

	m_gridInterrupts = new ui::GridView();
	m_gridInterrupts->create(splitter2, ui::GridView::WsColumnHeader);
	m_gridInterrupts->addColumn(new ui::GridColumn(L"Interrupt", 60_ut));
	m_gridInterrupts->addColumn(new ui::GridColumn(L"Count", 80_ut));

	m_gridThreads = new ui::GridView();
	m_gridThreads->create(splitter2, ui::GridView::WsColumnHeader);
	m_gridThreads->addColumn(new ui::GridColumn(L"Thread", 60_ut));
	m_gridThreads->addColumn(new ui::GridColumn(L"Count", 80_ut));
	m_gridThreads->addColumn(new ui::GridColumn(L"Wait", 80_ut));
	m_gridThreads->addColumn(new ui::GridColumn(L"Sleep", 80_ut));

	// Add rows for all registers.
	for (int32_t i = 0; i < 32; ++i)
		m_gridRegisters->addRow({ getRegisterName(i), L"" });

	// Add rows for all interrupts.
	m_gridInterrupts->addRow({ L"Timer", L"" });
	m_gridInterrupts->addRow({ L"Input", L"" });
	m_gridInterrupts->addRow({ L"Video", L"" });
	m_gridInterrupts->addRow({ L"Audio", L"" });
	
	// Add rows for all threads.
	for (int32_t i = 0; i < 16; ++i)
		m_gridThreads->addRow({ str(L"%d", i), L"", L"", L"" });

	update();
	show();

    return true;
}

void MainForm::updateVideo()
{
    drawing::Image* videoImage = m_emulator->getVideo()->getImage();
    if (!videoImage)
        return;

    if (!m_uiImage)
        return;

    const ui::Size sz = m_uiImage->getSize(this);
    if (sz.cx != videoImage->getWidth() || sz.cy != videoImage->getHeight())
    {
        m_uiImage->destroy();
        m_uiImage->create(videoImage);
    }
    else
        m_uiImage->copyImage(videoImage);

    m_image->setImage(m_uiImage);

	// Update registers.
	const ICPU* cpu = m_emulator->getCPU();
	for (int32_t i = 0; i < 32; ++i)
	{
		const uint32_t value = cpu->getRegister(i);
		ui::GridRow* row = m_gridRegisters->getRow(i);
		row->set(1, str(L"%08x", value));
	}
	m_gridRegisters->requestUpdate();

	// Update interrupt counters.
	const uint32_t* irqCounters = m_emulator->getIRQCounters();
	for (int32_t i = 0; i < 4; ++i)
	{
		ui::GridRow* row = m_gridInterrupts->getRow(i);
		row->set(1, str(L"%d", irqCounters[i]));
	}
	m_gridInterrupts->requestUpdate();

	// Update thread counters.
	const uint32_t* threadActiveCounters = m_emulator->getThreadActiveCounters();
	const uint32_t* threadWaitingCounters = m_emulator->getThreadWaitingCounters();
	const uint32_t* threadSleepingCounters = m_emulator->getThreadSleepingCounters();
	for (int32_t i = 0; i < 16; ++i)
	{
		ui::GridRow* row = m_gridThreads->getRow(i);
		row->set(1, str(L"%d", threadActiveCounters[i]));
		row->set(2, str(L"%d", threadWaitingCounters[i]));
		row->set(3, str(L"%d", threadSleepingCounters[i]));
	}
	m_gridThreads->requestUpdate();
}

void MainForm::imageMouseButtonDown(traktor::ui::MouseButtonDownEvent* event)
{
	if (event->getButton() == ui::MbtLeft)
		m_emulator->inputButton(true);
}

void MainForm::imageMouseButtonUp(traktor::ui::MouseButtonUpEvent* event)
{
	if (event->getButton() == ui::MbtLeft)
		m_emulator->inputButton(false);
}

void MainForm::imageMouseMove(traktor::ui::MouseMoveEvent* event)
{
	const ui::Size sz = m_image->getInnerRect().getSize();
	const ui::Point pt = event->getPosition();
	
	const float fptx = (float)pt.x / (sz.cx / 180.0f);
	const float fpty = (float)pt.y / (sz.cy / 180.0f);

	m_dptx += fptx - m_lptx;
	m_dpty += fpty - m_lpty;

	m_lptx = fptx;
	m_lpty = fpty;

	const int32_t idptx = (int32_t)m_dptx;
	const int32_t idpty = (int32_t)m_dpty;

	m_dptx -= idptx;
	m_dpty -= idpty;

	m_emulator->inputMovement(idptx, idpty);
}

void MainForm::imageKeyDown(traktor::ui::KeyDownEvent* event)
{
	switch (event->getVirtualKey())
	{
	case ui::Vk1:
		m_emulator->inputSetBit(3, true);
		break;
	case ui::Vk2:
		m_emulator->inputSetBit(2, true);
		break;
	case ui::Vk3:
		m_emulator->inputSetBit(1, true);
		break;
	case ui::Vk4:
		m_emulator->inputSetBit(0, true);
		break;
	case ui::Vk5:
		m_emulator->inputSetBit(8, true);
		break;
	case ui::Vk6:
		m_emulator->inputSetBit(9, true);
		break;
	case ui::VkW:
		m_emulator->inputSetBit(4, true);
		break;
	case ui::VkS:
		m_emulator->inputSetBit(5, true);
		break;
	case ui::VkD:
		m_emulator->inputSetBit(6, true);
		break;
	case ui::VkA:
		m_emulator->inputSetBit(7, true);
		break;
	}
}

void MainForm::imageKeyUp(traktor::ui::KeyUpEvent* event)
{
	if (event->isRepeat())
		return;
	switch (event->getVirtualKey())
	{
	case ui::Vk1:
		m_emulator->inputSetBit(3, false);
		break;
	case ui::Vk2:
		m_emulator->inputSetBit(2, false);
		break;
	case ui::Vk3:
		m_emulator->inputSetBit(1, false);
		break;
	case ui::Vk4:
		m_emulator->inputSetBit(0, false);
		break;
	case ui::Vk5:
		m_emulator->inputSetBit(8, false);
		break;
	case ui::Vk6:
		m_emulator->inputSetBit(9, false);
		break;
	case ui::VkW:
		m_emulator->inputSetBit(4, false);
		break;
	case ui::VkS:
		m_emulator->inputSetBit(5, false);
		break;
	case ui::VkD:
		m_emulator->inputSetBit(6, false);
		break;
	case ui::VkA:
		m_emulator->inputSetBit(7, false);
		break;
	}
}
