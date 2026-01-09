#include <Drawing/Image.h>
#include <Ui/Application.h>
#include <Ui/AspectLayout.h>
#include <Ui/Form.h>
#include <Ui/FloodLayout.h>
#include <Ui/TableLayout.h>
#include <Ui/StyleBitmap.h>
#include <Ui/StyleSheet.h>
#include <Ui/StatusBar/StatusBar.h>
#include <Ui/ToolBar/ToolBar.h>
#include <Ui/ToolBar/ToolBarButton.h>
#include <Ui/ToolBar/ToolBarButtonClickEvent.h>

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
	if (!ui::Form::create(L"RetroDACK", 620_ut, 420_ut, ui::Form::WsDefault, new ui::TableLayout(L"100%", L"*,100%", 0_ut, 0_ut)))
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

	Ref< ui::Container > container = new ui::Container();
	container->create(this, ui::WsNone, new ui::AspectLayout());
	
	m_uiImage = new ui::Bitmap(720, 720);
	m_image = new ui::Image();
	m_image->create(container, m_uiImage, ui::Image::WsScale | ui::Image::WsNearestFilter);
	m_image->addEventHandler< ui::MouseButtonDownEvent >([&](ui::MouseButtonDownEvent* event) {
		if (event->getButton() == ui::MbtLeft)
			m_emulator->inputButton(true);
	});
	m_image->addEventHandler< ui::MouseButtonUpEvent >([&](ui::MouseButtonUpEvent* event) {
		if (event->getButton() == ui::MbtLeft)
			m_emulator->inputButton(false);
	});
	m_image->addEventHandler< ui::MouseMoveEvent >([&](ui::MouseMoveEvent* event) {
		const ui::Size sz = m_image->getInnerRect().getSize();
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

        m_emulator->inputMovement(idptx, idpty);
	});
	m_image->addEventHandler< ui::KeyDownEvent >([&](ui::KeyDownEvent* event) {
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
	});
	m_image->addEventHandler< ui::KeyUpEvent >([&](ui::KeyUpEvent* event){
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
	});

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
}
