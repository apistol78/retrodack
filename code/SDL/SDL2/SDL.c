#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Runtime/Runtime.h>
#include <Runtime/Video.h>

#include "SDL2/SDL.h"

SDL_Surface* SDL_CreateRGBSurface(Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
    printf("SDL_CreateRGBSurface %d * %d, %d, R %08x, G %08x, B %08x, A %08x\n", width, height, depth, Rmask, Gmask, Bmask, Amask);

    SDL_Surface* surface = (SDL_Surface*)malloc(sizeof(SDL_Surface));

    surface->flags = 0;
    surface->format = 0;
    surface->w = width;
    surface->h = height;
    surface->pitch = width * 4;
    surface->pixels = malloc(width * height * 4);
    surface->userdata = 0;
    surface->locked = 0;
    surface->list_blitmap = 0;
    surface->refcount = 0;

    return surface;
}

void SDL_FreeSurface(SDL_Surface* surface)
{
    printf("SDL_FreeSurface\n");
    free(surface->pixels);
    free(surface);
}

int SDL_SetSurfaceBlendMode(SDL_Surface *surface, SDL_BlendMode blendMode)
{
    printf("SDL_SetSurfaceBlendMode\n");
    return 0;
}

Uint32 SDL_MapRGB(SDL_PixelFormat* format, Uint8 r, Uint8 g, Uint8 b)
{
    printf("SDL_MapRGB\n");
    return 0;
}

Uint32 SDL_MapRGBA(SDL_PixelFormat* format, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    printf("SDL_MapRGBA\n");
    return 0;
}

int SDL_FillRect(SDL_Surface *dst, SDL_Rect *dstrect, Uint32 color)
{
    printf("SDL_FillRect\n");
    return 0;
}

int SDL_BlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect)
{
    printf("SDL_BlitSurface\n");
    return 0;
}

SDL_Surface* SDL_ConvertSurface(SDL_Surface* src, const SDL_PixelFormat* fmt, Uint32 flags)
{
    printf("SDL_ConvertSurface %d * %d\n", src->w, src->h);
    SDL_Surface* surface = SDL_CreateRGBSurface(0, src->w, src->h, 0, 0, 0, 0, 0);
    return surface;
}

int SDL_SetSurfacePalette(SDL_Surface* surface, SDL_Palette* palette)
{
    printf("SDL_SetSurfacePalette\n");
    return 0;
}

int SDL_SetColorKey(SDL_Surface* surface, int flag, Uint32 key)
{
    printf("SDL_SetColorKey\n");
    return 0;
}

int SDL_SetSurfaceAlphaMod(SDL_Surface* surface, Uint8 alpha)
{
    printf("SDL_SetSurfaceAlphaMod\n");
    return 0;
}

SDL_bool SDL_SetClipRect(SDL_Surface * surface, const SDL_Rect * rect)
{
    printf("SDL_SetClipRect\n");
    return 0;
}

int SDL_LockSurface(SDL_Surface* surface)
{
    printf("SDL_LockSurface\n");
    return 0;
}

void SDL_UnlockSurface(SDL_Surface* surface)
{
    printf("SDL_UnlockSurface\n");
}

int SDL_GetRendererOutputSize(SDL_Renderer * renderer, int *w, int *h)
{
    printf("SDL_GetRendererOutputSize\n");
    return 0;
}

SDL_Texture* SDL_CreateTexture(SDL_Renderer * renderer, Uint32 format, int access, int w, int h)
{
    printf("SDL_CreateTexture %d * %d, format %d\n", w, h, format);
    SDL_Texture* texture = (SDL_Texture*)malloc(sizeof(SDL_Texture));
    texture->w = w;
    texture->h = h;
    texture->pixels = malloc(w * h * 4);
    return texture;
}

int SDL_UpdateTexture(SDL_Texture * texture, const SDL_Rect * rect, const void *pixels, int pitch)
{
    if (rect)
        printf("SDL_UpdateTexture %d * %d - %d * %d\n", rect->x, rect->y, rect->w, rect->h);
    else
        printf("SDL_UpdateTexture\n");

    memcpy(texture->pixels, pixels, texture->w * texture->h);

    return 0;
}

void SDL_RenderGetScale(SDL_Renderer * renderer, float *scaleX, float *scaleY)
{
    printf("SDL_RenderGetScale\n");
}

void SDL_RenderGetLogicalSize(SDL_Renderer * renderer, int *w, int *h)
{
    printf("SDL_RenderGetLogicalSize\n");
}

void SDL_RenderGetViewport(SDL_Renderer * renderer, SDL_Rect * rect)
{
    printf("SDL_RenderGetViewport\n");
}

Uint32 SDL_GetMouseState(int *x, int *y)
{
    printf("SDL_GetMouseState\n");
    return 0;
}

const Uint8* SDL_GetKeyboardState(int* numkeys)
{
    printf("SDL_GetKeyboardState\n");
    return 0;
}

int SDL_SetWindowFullscreen(SDL_Window * window, Uint32 flags)
{
    printf("SDL_SetWindowFullscreen\n");
    return 0;
}

int SDL_RenderSetIntegerScale(SDL_Renderer * renderer, SDL_bool enable)
{
    printf("SDL_RenderSetIntegerScale\n");
    return 0;
}

int SDL_RenderSetLogicalSize(SDL_Renderer* renderer, int w, int h)
{
    printf("SDL_RenderSetLogicalSize %d * %d\n", w, h);
    return 0;
}

void * SDL_memset(void* dst, int c, size_t len)
{
    printf("SDL_memset\n");
    return 0;
}

int SDL_OpenAudio(SDL_AudioSpec * desired, SDL_AudioSpec * obtained)
{
    printf("SDL_OpenAudio\n");
    return 0;
}

void SDL_LockAudio()
{
    printf("SDL_LockAudio\n");
}

void SDL_UnlockAudio()
{
    printf("SDL_UnlockAudio\n");
}

void SDL_PauseAudio(int pause_on)
{
    printf("SDL_PauseAudio\n");
}

SDL_RWops* SDL_RWFromConstMem(const void* mem, int size)
{
    printf("SDL_RWFromConstMem\n");
    return 0;
}

SDL_RWops* SDL_RWFromFile(const char *file, const char *mode)
{
    printf("SDL_RWFromFile\n");
    return 0;
}

int SDL_RWclose(SDL_RWops* context)
{
    printf("SDL_RWclose\n");
    return 0;
}

size_t SDL_RWwrite(SDL_RWops* context, const void* ptr, size_t size, size_t num)
{
    printf("SDL_RWwrite\n");
    return 0;
}

size_t SDL_RWread(SDL_RWops* context, void* ptr, size_t size, size_t maxnum)
{
    printf("SDL_RWread\n");
    return 0;
}

void SDL_GetVersion(SDL_version* ver)
{
    printf("SDL_GetVersion\n");
}

SDL_TimerID SDL_AddTimer(Uint32 interval, SDL_TimerCallback callback, void* param)
{
    printf("SDL_AddTimer\n");
    return 0;
}

Uint64 SDL_GetPerformanceCounter()
{
    printf("SDL_GetPerformanceCounter\n");
    return 0;
}

Uint64 SDL_GetPerformanceFrequency()
{
    printf("SDL_GetPerformanceFrequency\n");
    return 0;
}

void SDL_Delay(Uint32 ms)
{
    printf("SDL_Delay\n");
}

int SDL_SetPaletteColors(SDL_Palette* palette, const SDL_Color* colors, int firstcolor, int ncolors)
{
    printf("SDL_SetPaletteColors\n");
    return 0;
}

int SDL_HapticRumblePlay(SDL_Haptic* haptic, float strength, Uint32 length)
{
    printf("SDL_HapticRumblePlay\n");
    return 0;
}

int SDL_JoystickRumble(SDL_Joystick* joystick, Uint16 lfr, Uint16 hfr, Uint32 duration)
{
    printf("SDL_JoystickRumble\n");
    return 0;
}

int SDL_GameControllerRumble(SDL_GameController* controller, Uint16 lft, Uint16 hfr, Uint32 duration)
{
    printf("SDL_GameControllerRumble\n");
    return 0;
}

int SDL_NumJoysticks()
{
    printf("SDL_NumJoysticks\n");
    return 0;
}

SDL_Joystick* SDL_JoystickOpen(int device_index)
{
    printf("SDL_JoystickOpen\n");
    return 0;
}

int SDL_GameControllerAddMappingsFromRW(SDL_RWops * rw, int freerw)
{
    printf("SDL_GameControllerAddMappingsFromRW\n");
    return 0;
}

SDL_GameController* SDL_GameControllerOpen(int joystick_index)
{
    printf("SDL_GameControllerOpen\n");
    return 0;
}

SDL_bool SDL_IsGameController(int joystick_index)
{
    printf("SDL_IsGameController\n");
    return 0;
}

const char* SDL_GetError()
{
    printf("SDL_GetError\n");
    return 0;
}

void SDL_Quit()
{
    printf("SDL_Quit\n");
    for (;;);
}

SDL_Haptic* SDL_HapticOpen(int device_index)
{
    printf("SDL_HapticOpen\n");
    return 0;
}

int SDL_HapticRumbleInit(SDL_Haptic * haptic)
{
    printf("SDL_HapticRumbleInit\n");
    return 0;
}

void SDL_SetTextInputRect(const SDL_Rect *rect)
{
    printf("SDL_SetTextInputRect\n");
}

void SDL_StartTextInput(void)
{
    printf("SDL_StartTextInput\n");
}

void SDL_StopTextInput(void)
{
    printf("SDL_StopTextInput\n");
}

int SDL_PushEvent(SDL_Event * event)
{
    printf("SDL_PushEvent\n");
    return 0;
}

int SDL_PollEvent(SDL_Event * event)
{
    printf("SDL_PollEvent\n");
    return 0;
}

SDL_bool SDL_SetHint(const char *name, const char *value)
{
    printf("SDL_SetHint\n");
    return 0;
}

int SDL_Init(Uint32 flags)
{
    runtime_init();
    rt_video_set_mode(VMODE_320_200_8);
    return 0;
}

int SDL_InitSubSystem(Uint32 flags)
{
    printf("SDL_InitSubSystem\n");
    return 0;
}

SDL_Window * SDL_CreateWindow(const char *title, int x, int y, int w, int h, Uint32 flags)
{
    printf("SDL_CreateWindow\n");
    return 0;
}

void SDL_SetWindowIcon(SDL_Window * window, SDL_Surface * icon)
{
    printf("SDL_SetWindowIcon\n");
}

Uint32 SDL_GetWindowFlags(SDL_Window * window)
{
    printf("SDL_GetWindowFlags\n");
    return 0;
}

SDL_Renderer * SDL_CreateRenderer(SDL_Window* window, int index, Uint32 flags)
{
    printf("SDL_CreateRenderer\n");
    SDL_Renderer* renderer = (SDL_Renderer*)malloc(sizeof(SDL_Renderer));
    return renderer;
}

int SDL_GetRendererInfo(SDL_Renderer* renderer, SDL_RendererInfo* info)
{
    printf("SDL_GetRendererInfo\n");
    info->name = "RetroDACK";
    info->flags = 0;
    info->num_texture_formats = 0;
    info->max_texture_width = 0;
    info->max_texture_height = 0;
    return 0;
}

int SDL_ShowCursor(int toggle)
{
    printf("SDL_ShowCursor\n");
    return 0;
}

int SDL_SetRenderTarget(SDL_Renderer *renderer, SDL_Texture *texture)
{
    printf("SDL_SetRenderTarget\n");
    return 0;
}

int SDL_RenderClear(SDL_Renderer * renderer)
{
    printf("SDL_RenderClear\n");
    rt_video_clear(0);
    rt_video_wait();
    return 0;
}

int SDL_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture, const SDL_Rect * srcrect, const SDL_Rect * dstrect)
{
    printf("SDL_RenderCopy\n");
    void* fb = rt_video_get_secondary_target();
    memcpy(fb, texture->pixels, texture->w * texture->h);
    return 0;
}

int SDL_BlitScaled(const SDL_Surface *src, const SDL_Rect *srcrect, SDL_Surface *dst, const SDL_Rect *dstrect)
{
    printf("SDL_BlitScaled\n");
    return 0;
}

void SDL_RenderPresent(SDL_Renderer * renderer)
{
    printf("SDL_RenderPresent\n");
    rt_video_present();
}

SDL_Surface* SDL_ConvertSurfaceFormat(SDL_Surface * src, Uint32 pixel_format, Uint32 flags)
{
    printf("SDL_ConvertSurfaceFormat %d * %d\n", src->w, src->h);
    SDL_Surface* surface = SDL_CreateRGBSurface(0, src->w, src->h, 0, 0, 0, 0, 0);
    return surface;
}
