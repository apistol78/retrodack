#pragma once

#include <Runtime/Video.h>

namespace sn
{

class VirtualScreen
{
public:
    constexpr static uint32_t CenterOffset = (360 - 256) / 2 + ((360 - 240) / 2) * 360;

    void create(unsigned int width, unsigned int height, float pixel_size, uint32_t color);

    void setPixel(uint32_t offset, uint8_t color) { m_buffer[CenterOffset + offset] = color; }

    void present();

private:
    uint8_t* m_buffer = nullptr;
};

}
