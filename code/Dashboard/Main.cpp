#include <cstdlib>
#include <ctime>
#include <iostream>

#include <SDL2/SDL.h>

int main()
{
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_Renderer* renderer = SDL_CreateRenderer(nullptr, 0, 0);

	SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, 320, 200, 8, SDL_PIXELFORMAT_INDEX8);

   	SDL_Color colors[256];
    for (int i = 0; i < 256; i++)
	{
        colors[i].r = (i & 0xE0);       // 3 bits red
        colors[i].g = (i & 0x1C) << 3;  // 3 bits green
        colors[i].b = (i & 0x03) << 6;  // 2 bits blue
        colors[i].a = 255;
    }
    SDL_SetPaletteColors(surface->format->palette, colors, 0, 256);

	SDL_Texture* texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		320,
		200
	);

	for (;;)
	{
        Uint8* pixels = static_cast< Uint8* >(surface->pixels);
        for (int i = 0; i < 320 * 200; i++)
		{
            pixels[i] = std::rand() % 256;
        }

		SDL_RenderClear(renderer);
		SDL_Ex_RenderSetPalette(renderer, surface->format->palette);
		SDL_Ex_RenderCopySurface(renderer, surface);
		SDL_RenderPresent(renderer);
	}

	return 0;
}
