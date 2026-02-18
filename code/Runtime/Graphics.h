/*
 RetroDÄCK
 Copyright (c) 2025-2026 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#pragma once

#include <HAL/Common.h>

typedef struct
{
    int32_t width;
    int32_t height;
    uint8_t* pixels;
}
rt_gfx_context_t;

typedef union
{
    struct
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t x;
    };
    uint32_t dw;
}
rt_gfx_color_t;

typedef struct
{
    rt_gfx_color_t colors[256];
    int32_t minIndex;
    int32_t maxIndex;
}
rt_gfx_palette_t;

typedef struct
{
    int32_t width;
    int32_t height;
    uint8_t* pixels;
    rt_gfx_palette_t* palette;
}
rt_gfx_image_t;

EXTERN_C rt_gfx_image_t* rt_gfx_create_image(int32_t width, int32_t height);

EXTERN_C rt_gfx_image_t* rt_gfx_create_external_image(int32_t width, int32_t height, const uint8_t* bits);

EXTERN_C void rt_gfx_destroy_image(rt_gfx_image_t* image);

EXTERN_C rt_gfx_palette_t* rt_gfx_create_palette();

EXTERN_C void rt_gfx_destroy_palette(rt_gfx_palette_t* palette);

EXTERN_C rt_gfx_image_t* rt_gfx_load_image(const char* filename);

EXTERN_C void rt_gfx_blit_image(rt_gfx_context_t* ctx, const rt_gfx_image_t* image, int32_t x, int32_t y);

EXTERN_C void rt_gfx_blit_image_key(rt_gfx_context_t* ctx, const rt_gfx_image_t* image, int32_t x, int32_t y);

EXTERN_C void rt_gfx_blit_image_region(rt_gfx_context_t* ctx, const rt_gfx_image_t* image, int32_t srcX, int32_t srcY, int32_t width, int32_t height, int32_t destX, int32_t destY);

EXTERN_C void rt_gfx_draw_hline(rt_gfx_context_t* ctx, int32_t x1, int32_t x2, int32_t y, uint8_t color);

EXTERN_C void rt_gfx_draw_vline(rt_gfx_context_t* ctx, int32_t x, int32_t y1, int32_t y2, uint8_t color);

EXTERN_C void rt_gfx_draw_rect(rt_gfx_context_t* ctx, int32_t x, int32_t y, int32_t width, int32_t height, uint8_t color);

EXTERN_C void rt_gfx_fill_rect(rt_gfx_context_t* ctx, int32_t x, int32_t y, int32_t width, int32_t height, uint8_t color);

EXTERN_C void rt_gfx_draw_line(rt_gfx_context_t* ctx, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint8_t color);

EXTERN_C void rt_gfx_draw_char(rt_gfx_context_t* ctx, const void* font, int32_t x, int32_t y, char ch, uint8_t color);

EXTERN_C void rt_gfx_draw_string(rt_gfx_context_t* ctx, const void* font, int32_t x, int32_t y, const char* str, uint8_t color);


