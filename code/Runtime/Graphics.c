/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include <stdlib.h>

#include "Runtime/Graphics.h"

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
	return image;
}

void rt_gfx_destroy_image(rt_gfx_image_t* image)
{
	free(image);
}

rt_gfx_image_t* rt_gfx_load_image(const char* filename)
{
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
