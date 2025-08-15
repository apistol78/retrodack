#pragma once

#include "SDL2/SDL.h"

#define IMG_GetError SDL_GetError

SDL_Surface* IMG_Load(const char *file);

SDL_Surface* IMG_Load_RW(SDL_RWops* src, int freesrc);
