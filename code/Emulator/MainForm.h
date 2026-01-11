#pragma once

#include <Core/Ref.h>
#include <Ui/Bitmap.h>
#include <Ui/Image.h>
#include <Ui/Form.h>
#include <Ui/GridView/GridView.h>

class Emulator;

class MainForm : public traktor::ui::Form
{
	T_RTTI_CLASS;

public:
	explicit MainForm(Emulator* emulator);

	bool create();

	void updateVideo();

private:
	traktor::Ref< Emulator > m_emulator;
	traktor::Ref< traktor::ui::Bitmap > m_uiImage;
	traktor::Ref< traktor::ui::Image > m_image;
	traktor::Ref< traktor::ui::GridView > m_gridRegisters;
	float m_lptx = 0.0f;
	float m_lpty = 0.0f;
	float m_dptx = 0.0f;
	float m_dpty = 0.0f;

	void imageMouseButtonDown(traktor::ui::MouseButtonDownEvent* event);

	void imageMouseButtonUp(traktor::ui::MouseButtonUpEvent* event);

	void imageMouseMove(traktor::ui::MouseMoveEvent* event);

	void imageKeyDown(traktor::ui::KeyDownEvent* event);

	void imageKeyUp(traktor::ui::KeyUpEvent* event);
};
