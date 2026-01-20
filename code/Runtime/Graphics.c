/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include <stdio.h>
#include <stdlib.h>

#include "Runtime/File.h"
#include "Runtime/Graphics.h"

#pragma pack(1)
struct PCXHeader
{
	uint8_t manufacturer;
	uint8_t version;
	uint8_t encoding;
	uint8_t bitsPerPixel;
	uint16_t xmin;
	uint16_t ymin;
	uint16_t xmax;
	uint16_t ymax;
	uint16_t vdpi;
	uint8_t palette[48];
	uint8_t reserved;
	uint8_t planes;
	uint16_t pitch;
	uint16_t paletteType;
	uint16_t screenWidth;
	uint16_t screenHeight;
	uint8_t dummy[56];
};
#pragma pack()

static int32_t min(int32_t a, int32_t b)
{
	return a < b ? a : b;
}

static int32_t max(int32_t a, int32_t b)
{
	return a > b ? a : b;
}

rt_gfx_image_t* rt_gfx_create_image(int32_t width, int32_t height)
{
	rt_gfx_image_t* image = (rt_gfx_image_t*)malloc(sizeof(rt_gfx_image_t) + width * height);
	if (!image)
		return 0;

	image->width = width;
	image->height = height;
	image->pixels = (uint8_t*)(image + 1);
	image->palette = 0;
	return image;
}

void rt_gfx_destroy_image(rt_gfx_image_t* image)
{
	free(image);
}

rt_gfx_palette_t* rt_gfx_create_palette()
{
	rt_gfx_palette_t* palette = (rt_gfx_palette_t*)malloc(sizeof(rt_gfx_palette_t));
	if (!palette)
		return 0;

	palette->minIndex = 0;
	palette->maxIndex = 0;
	return palette;
}

void rt_gfx_destroy_palette(rt_gfx_palette_t* palette)
{
	free(palette);
}

rt_gfx_image_t* rt_gfx_load_image(const char* filename)
{
	struct PCXHeader hdr;

	const int32_t fd = file_open(filename, FILE_MODE_READ);
	if (fd <= 0)
		return 0;

	file_read(fd, &hdr, sizeof(hdr));

	if (hdr.bitsPerPixel != 8)
		goto cleanup;

	rt_gfx_image_t* image = rt_gfx_create_image(hdr.xmax - hdr.xmin + 1, hdr.ymax - hdr.ymin + 1);
	if (!image)
		goto cleanup;

	for (int32_t y = 0; y < image->height; ++y)
	{
		uint8_t* scan = &image->pixels[y * image->width];

		int32_t count = 0;
		uint8_t value = 0;

		int32_t x = image->width;
		while (x > 0)
		{
			uint8_t c;
			if (file_read(fd, &c, sizeof(uint8_t)) != sizeof(uint8_t))
			{
				rt_gfx_destroy_image(image);
				goto cleanup;
			}
			if ((c & 0xc0) == 0xc0)
			{
				count = c & 0x3f;
				if (file_read(fd, &value, sizeof(uint8_t)) != sizeof(uint8_t))
				{
					rt_gfx_destroy_image(image);
					goto cleanup;
				}
			}
			else
			{
				count = 1;
				value = c;
			}

			if (count > x)
				count = x;

			x -= count;

			while (count-- > 0)
				*scan++ = value;
		}
	}

	uint8_t dummy;
	if (file_read(fd, &dummy, sizeof(uint8_t)) != sizeof(uint8_t))
	{
		rt_gfx_destroy_image(image);
		goto cleanup;
	}	

	image->palette = rt_gfx_create_palette();
	image->palette->minIndex = 0;
	image->palette->maxIndex = 255;

	for (uint32_t i = 0; i < 256; ++i)
	{
		uint8_t rgb[3];
		file_read(fd, rgb, 3 * sizeof(uint8_t));
		image->palette->colors[i].b = rgb[0];
		image->palette->colors[i].g = rgb[1];
		image->palette->colors[i].r = rgb[2];
		image->palette->colors[i].x = 255;
	} 

	file_close(fd);
	return image;

cleanup:
	file_close(fd);
	return 0;
}

void rt_gfx_blit_image(rt_gfx_context_t* ctx, const rt_gfx_image_t* image, int32_t x, int32_t y)
{
	const int32_t w = ctx->width;
	const int32_t h = ctx->height;

	const int32_t ox = max(0, -x);
	const int32_t oy = max(0, -y);

	const int32_t dw = min(image->width - ox, w - x);
	const int32_t dh = min(image->height - oy, h - y);
	if (dw <= 0 || dh <= 0)
		return;

	const uint8_t* sp = image->pixels + oy * image->width + ox;
	uint8_t* dp = ctx->pixels + (y + oy) * w + (x + ox);
	for (int32_t iy = 0; iy < dh; ++iy)
	{
		for (int32_t ix = 0; ix < dw; ++ix)
		{
			dp[ix] = sp[ix];
		}
		sp += image->width;
		dp += w;
	}
}

void rt_gfx_blit_image_region(rt_gfx_context_t* ctx, const rt_gfx_image_t* image, int32_t srcX, int32_t srcY, int32_t width, int32_t height, int32_t destX, int32_t destY)
{
	const int32_t w = ctx->width;
	const int32_t h = ctx->height;

	const int32_t ox = max(0, -destX);
	const int32_t oy = max(0, -destY);

	const int32_t dw = min(width - ox, w - destX);
	const int32_t dh = min(height - oy, h - destY);
	if (dw <= 0 || dh <= 0)
		return;

	const uint8_t* sp = image->pixels + srcY * image->width + srcX;
	uint8_t* dp = ctx->pixels + (destY + oy) * w + (destX + ox);
	for (int32_t iy = 0; iy < dh; ++iy)
	{
		for (int32_t ix = 0; ix < dw; ++ix)
		{
			dp[ix] = sp[ix];
		}
		sp += image->width;
		dp += w;
	}	
}

void rt_gfx_draw_hline(rt_gfx_context_t* ctx, int32_t x1, int32_t x2, int32_t y, uint8_t color)
{
	const int32_t w = ctx->width;
	const int32_t h = ctx->height;

	if (y < 0 || y >= h)
		return;

	if (x1 < 0)
		x1 = 0;
	else if (x1 > w - 1)
		x1 = w - 1;
	if (x2 < 0)
		x2 = 0;
	else if (x2 > w - 1)
		x2 = w - 1;

	if (x1 > x2)
	{
		int32_t tmp = x1;
		x1 = x2;
		x2 = tmp;
	}

	uint8_t* dp = ctx->pixels + y * w + x1;
	for (int32_t x = x1; x <= x2; ++x)
		*dp++ = color;
}

void rt_gfx_draw_vline(rt_gfx_context_t* ctx, int32_t x, int32_t y1, int32_t y2, uint8_t color)
{
	const int32_t w = ctx->width;
	const int32_t h = ctx->height;

	if (x < 0 || x >= w)
		return;

	if (y1 < 0)
		y1 = 0;
	else if (y1 > h - 1)
		y1 = w - 1;
	if (y2 < 0)
		y2 = 0;
	else if (y2 > h - 1)
		y2 = w - 1;

	if (y1 > y2)
	{
		int32_t tmp = y1;
		y1 = y2;
		y2 = tmp;
	}

	uint8_t* dp = ctx->pixels + x + y1 * w;
	for (int32_t y = y1; y <= y2; ++y)
	{
		*dp = color;	
		dp += w;
	}
}

void rt_gfx_draw_rect(rt_gfx_context_t* ctx, int32_t x, int32_t y, int32_t width, int32_t height, uint8_t color)
{
	rt_gfx_draw_hline(ctx, x, x + width - 1, y, color);
	rt_gfx_draw_hline(ctx, x, x + width - 1, y + height - 1, color);
	rt_gfx_draw_vline(ctx, x, y, y + height - 1, color);
	rt_gfx_draw_vline(ctx, x + width - 1, y, y + height - 1, color);
}

void rt_gfx_fill_rect(rt_gfx_context_t* ctx, int32_t x, int32_t y, int32_t width, int32_t height, uint8_t color)
{
	const int32_t w = ctx->width;
	const int32_t h = ctx->height;

	if (x < 0)
	{
		width += x;
		x = 0;
	}
	if (y < 0)
	{
		height += y;
		y = 0;
	}

	const int32_t dw = min(width, w - x);
	const int32_t dh = min(height, h - y);
	if (dw <= 0 || dh <= 0)
		return;

	uint8_t* dp = ctx->pixels + y * w + x;
	for (int32_t iy = 0; iy < dh; ++iy)
	{
		for (int32_t ix = 0; ix < dw; ++ix)
		{
			dp[ix] = color;
		}
		dp += w;
	}			
}

void rt_gfx_draw_line(rt_gfx_context_t* ctx, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint8_t color)
{
}

void rt_gfx_draw_char(rt_gfx_context_t* ctx, const void* font, int32_t x, int32_t y, char ch, uint8_t color)
{
	const int32_t w = ctx->width;
	uint8_t* dp = ctx->pixels + y * w + x;
	for (int32_t y = 0; y < 8; ++y)
	{
		for (int32_t x = 0; x < 8; ++x)
		{
			if (((const uint8_t*)font)[(ch - ' ') * 8 + x] & (1 << y))
				dp[x] = color;
		}
		dp += w;
	}
}
