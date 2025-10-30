#include <cstdlib>
#include <cstring>

#include <Runtime/Video.h>

#include "PaletteColors.h"
#include "VirtualScreen.h"

namespace sn
{

void VirtualScreen::create(unsigned int w, unsigned int h, float pixel_size, uint32_t color)
{
	rt_video_set_mode(VMODE_360_360_8);
    for (int i = 0; i < sizeof(c_paletteColors) / sizeof(c_paletteColors[0]); ++i)
    {
        const uint8_t* p = (const uint8_t*)&c_paletteColors[i];
        const uint32_t rgb = (p[3] << 16) | (p[2] << 8) | (p[1] << 0);
        rt_video_set_palette(i, rgb);
    }

    m_buffer = (uint8_t*)malloc(360 * 360);
    memset(m_buffer, 0, 360 * 360);
}

void VirtualScreen::present()
{
    rt_video_blit(m_buffer);
	rt_video_wait();
	rt_video_present(0);
}

}
