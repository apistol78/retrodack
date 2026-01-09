#pragma once

#include <Core/Ref.h>
#include <Ui/Bitmap.h>
#include <Ui/Image.h>
#include <Ui/Form.h>

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
	float lptx = 0.0f;
    float lpty = 0.0f;
	float dptx = 0.0f;
    float dpty = 0.0f;
};
