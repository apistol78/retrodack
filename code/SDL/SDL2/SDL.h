#pragma once

#include <strings.h>
#include <stdint.h>

#define SDL_COMPILE_TIME_ASSERT(x, y)
#define SDL_VERSION_ATLEAST(a, b, c) (1)
#define SDL_VERSION(x) { (x)->major = 2; (x)->minor = 0; (x)->patch = 0; }
#define SDL_SwapBE32(x) (x)
#define SDL_SwapBE16(x) (x)
#define SDL_SwapLE16(x) (x)
#define SDL_SwapLE32(x) (x)
#define SDL_ISPIXELFORMAT_INDEXED(format) (0)
#define SDL_SURFACE_PREALLOCATED    0x00000001u /**< Surface uses preallocated pixel memory */
#define SDL_SURFACE_LOCK_NEEDED     0x00000002u /**< Surface needs to be locked to access pixels */
#define SDL_SURFACE_LOCKED          0x00000004u /**< Surface is currently locked */
#define SDL_SURFACE_SIMD_ALIGNED    0x00000008u /**< Surface uses pixel memory allocated with SDL_aligned_alloc() */

typedef int8_t Sint8;
typedef uint8_t Uint8;
typedef int16_t Sint16;
typedef uint16_t Uint16;
typedef int32_t Sint32;
typedef uint32_t Uint32;
typedef int64_t Sint64;
typedef uint64_t Uint64;

typedef enum
{
    SDL_FALSE = 0,
    SDL_TRUE = 1
}
SDL_bool;

typedef struct
{
    Uint8 major;
    Uint8 minor;
    Uint8 patch;
}
SDL_version;

typedef struct
{
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
}
SDL_Rect;

typedef struct
{
    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 a;
}
SDL_Color;

typedef struct
{
    int ncolors;
    SDL_Color* colors;
    Uint32 version;
    int refcount;
}
SDL_Palette;

typedef enum SDL_TextureAccess
{
    SDL_TEXTUREACCESS_STATIC,    /**< Changes rarely, not lockable */
    SDL_TEXTUREACCESS_STREAMING, /**< Changes frequently, lockable */
    SDL_TEXTUREACCESS_TARGET     /**< Texture can be used as a render target */
}
SDL_TextureAccess;

typedef enum
{
    SDL_PIXELFORMAT_RGB24,
    SDL_PIXELFORMAT_ARGB8888
}
SDL_PixelFormatEnum;

typedef struct
{
    Uint32 format;
    SDL_Palette* palette;
    Uint8 BitsPerPixel;
    Uint8 BytesPerPixel;
    Uint8 padding[2];
    Uint32 Rmask;
    Uint32 Gmask;
    Uint32 Bmask;
    Uint32 Amask;
    Uint8 Rloss;
    Uint8 Gloss;
    Uint8 Bloss;
    Uint8 Aloss;
    Uint8 Rshift;
    Uint8 Gshift;
    Uint8 Bshift;
    Uint8 Ashift;
    int refcount;
    struct SDL_PixelFormat *next;    
}
SDL_PixelFormat;

typedef Uint32 SDL_SurfaceFlags;

typedef struct
{
    Uint32 flags;
    SDL_PixelFormat* format;
    int w;
    int h;
    int pitch;
    void* pixels;
    void* userdata;
    int locked;
    void* list_blitmap;
    SDL_Rect clip_rect;
    //SDL_Blitmap* map;
    int refcount;
}
SDL_Surface;

typedef void* SDL_RWops;

typedef struct
{
    int w;
    int h;
    void* pixels;
}
SDL_Texture;

typedef void* SDL_GameController;

typedef void* SDL_Joystick;

typedef void* SDL_Haptic;

typedef void* SDL_Renderer;

typedef void* SDL_Window;

typedef struct
{
    int freq;
    int /* SDL_AudioFormat */ format;
    Uint8 channels;
    Uint8 silence;
    Uint16 samples;
    Uint16 padding;
    Uint32 size;
    void* /* SDL_AudioCallback */ callback;
    void* userdata;
}
SDL_AudioSpec;

#define SDL_SCANCODE_LEFT 0
#define SDL_SCANCODE_RIGHT 1
#define SDL_SCANCODE_UP 2
#define SDL_SCANCODE_DOWN 3
#define SDL_SCANCODE_HOME 4
#define SDL_SCANCODE_PAGEUP 5
#define SDL_SCANCODE_RSHIFT 6
#define SDL_SCANCODE_RETURN 7
#define SDL_SCANCODE_ESCAPE 8
#define SDL_SCANCODE_A 9
#define SDL_SCANCODE_R 10
#define SDL_SCANCODE_LSHIFT 11
#define SDL_SCANCODE_F9 12
#define SDL_SCANCODE_SPACE 13
#define SDL_SCANCODE_G 14
#define SDL_SCANCODE_J 15
#define SDL_SCANCODE_V 16
#define SDL_SCANCODE_C 17
#define SDL_SCANCODE_L 18
#define SDL_SCANCODE_K 19
#define SDL_SCANCODE_S 20
#define SDL_SCANCODE_F6 21
#define SDL_SCANCODE_KP_MINUS 22
#define SDL_SCANCODE_KP_PLUS 23
#define SDL_SCANCODE_I 24
#define SDL_SCANCODE_W 25
#define SDL_SCANCODE_H 26
#define SDL_SCANCODE_U 27
#define SDL_SCANCODE_N 28
#define SDL_SCANCODE_B 29
#define SDL_SCANCODE_T 30
#define SDL_SCANCODE_F 31
#define SDL_SCANCODE_KP_8 32
#define SDL_SCANCODE_KP_7 33
#define SDL_SCANCODE_KP_9 34
#define SDL_SCANCODE_CLEAR 35
#define SDL_SCANCODE_KP_5 36
#define SDL_SCANCODE_KP_2 37
#define SDL_SCANCODE_KP_4 38
#define SDL_SCANCODE_KP_6 39
#define SDL_SCANCODE_RIGHTBRACKET 40
#define SDL_SCANCODE_LEFTBRACKET 41
#define SDL_SCANCODE_TAB 42
#define SDL_SCANCODE_Q 43
#define SDL_SCANCODE_BACKSPACE 44
#define SDL_SCANCODE_DELETE 45
#define SDL_SCANCODE_GRAVE 46
#define SDL_SCANCODE_MUTE 47
#define SDL_SCANCODE_AUDIOMUTE 48
#define SDL_SCANCODE_PAUSE 49
#define SDL_SCANCODE_VOLUMEUP 50
#define SDL_SCANCODE_VOLUMEDOWN 51
#define SDL_SCANCODE_PRINTSCREEN 52
#define SDL_SCANCODE_APPLICATION 53
#define SDL_SCANCODE_NUMLOCKCLEAR 54
#define SDL_SCANCODE_SCROLLLOCK 55
#define SDL_SCANCODE_CAPSLOCK 56
#define SDL_SCANCODE_RGUI 57
#define SDL_SCANCODE_RALT 58
#define SDL_SCANCODE_RCTRL 59
#define SDL_SCANCODE_LGUI 60
#define SDL_SCANCODE_LALT 61
#define SDL_SCANCODE_LCTRL 62

#define SDL_NUM_SCANCODES 256

#define KMOD_ALT 1
#define KMOD_SHIFT 2
#define KMOD_CTRL 4

#define SDL_CONTROLLER_AXIS_LEFTX 0
#define SDL_CONTROLLER_AXIS_LEFTY 1
#define SDL_CONTROLLER_AXIS_RIGHTX 2
#define SDL_CONTROLLER_AXIS_RIGHTY 3
#define SDL_CONTROLLER_AXIS_TRIGGERLEFT 4
#define SDL_CONTROLLER_AXIS_TRIGGERRIGHT 5

SDL_Surface* SDL_CreateRGBSurface(Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);

void SDL_FreeSurface(SDL_Surface* surface);

typedef Uint32 SDL_BlendMode;

#define SDL_BLENDMODE_NONE                  0x00000000u /**< no blending: dstRGBA = srcRGBA */
#define SDL_BLENDMODE_BLEND                 0x00000001u /**< alpha blending: dstRGB = (srcRGB * srcA) + (dstRGB * (1-srcA)), dstA = srcA + (dstA * (1-srcA)) */
#define SDL_BLENDMODE_BLEND_PREMULTIPLIED   0x00000010u /**< pre-multiplied alpha blending: dstRGBA = srcRGBA + (dstRGBA * (1-srcA)) */
#define SDL_BLENDMODE_ADD                   0x00000002u /**< additive blending: dstRGB = (srcRGB * srcA) + dstRGB, dstA = dstA */
#define SDL_BLENDMODE_ADD_PREMULTIPLIED     0x00000020u /**< pre-multiplied additive blending: dstRGB = srcRGB + dstRGB, dstA = dstA */
#define SDL_BLENDMODE_MOD                   0x00000004u /**< color modulate: dstRGB = srcRGB * dstRGB, dstA = dstA */
#define SDL_BLENDMODE_MUL                   0x00000008u /**< color multiply: dstRGB = (srcRGB * dstRGB) + (dstRGB * (1-srcA)), dstA = dstA */
#define SDL_BLENDMODE_INVALID               0x7FFFFFFFu

#define SDL_ALPHA_TRANSPARENT 0x00
#define SDL_ALPHA_OPAQUE 0xff

int SDL_SetSurfaceBlendMode(SDL_Surface *surface, SDL_BlendMode blendMode);

Uint32 SDL_MapRGB(SDL_PixelFormat* format, Uint8 r, Uint8 g, Uint8 b);

Uint32 SDL_MapRGBA(SDL_PixelFormat* format, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

int SDL_FillRect(SDL_Surface *dst, SDL_Rect *dstrect, Uint32 color);

int SDL_BlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);

SDL_Surface* SDL_ConvertSurface(SDL_Surface* src, const SDL_PixelFormat* fmt, Uint32 flags);

int SDL_SetSurfacePalette(SDL_Surface* surface, SDL_Palette* palette);

int SDL_SetColorKey(SDL_Surface* surface, int flag, Uint32 key);

int SDL_SetSurfaceAlphaMod(SDL_Surface* surface, Uint8 alpha);

SDL_bool SDL_SetClipRect(SDL_Surface * surface, const SDL_Rect * rect);

int SDL_LockSurface(SDL_Surface* surface);

void SDL_UnlockSurface(SDL_Surface* surface);

int SDL_GetRendererOutputSize(SDL_Renderer * renderer, int *w, int *h);

SDL_Texture * SDL_CreateTexture(SDL_Renderer * renderer, Uint32 format, int access, int w, int h);

int SDL_UpdateTexture(SDL_Texture * texture, const SDL_Rect * rect, const void *pixels, int pitch);

void SDL_RenderGetScale(SDL_Renderer * renderer, float *scaleX, float *scaleY);

void SDL_RenderGetLogicalSize(SDL_Renderer * renderer, int *w, int *h);

void SDL_RenderGetViewport(SDL_Renderer * renderer, SDL_Rect * rect);

Uint32 SDL_GetMouseState(int *x, int *y);

const Uint8* SDL_GetKeyboardState(int* numkeys);

#define SDL_WINDOW_FULLSCREEN_DESKTOP 0

int SDL_SetWindowFullscreen(SDL_Window * window, Uint32 flags);

int SDL_RenderSetIntegerScale(SDL_Renderer * renderer, SDL_bool enable);

int SDL_RenderSetLogicalSize(SDL_Renderer * renderer, int w, int h);

void * SDL_memset(void* dst, int c, size_t len);


typedef Uint16 SDL_AudioFormat;

#define AUDIO_U8        0x0008 
#define AUDIO_S16SYS    0

int SDL_OpenAudio(SDL_AudioSpec * desired, SDL_AudioSpec * obtained);

void SDL_LockAudio();

void SDL_UnlockAudio();

void SDL_PauseAudio(int pause_on);


SDL_RWops* SDL_RWFromConstMem(const void* mem, int size);

SDL_RWops* SDL_RWFromFile(const char *file, const char *mode);

int SDL_RWclose(SDL_RWops* context);

size_t SDL_RWwrite(SDL_RWops* context, const void* ptr, size_t size, size_t num);

size_t SDL_RWread(SDL_RWops* context, void* ptr, size_t size, size_t maxnum);

void SDL_GetVersion(SDL_version* ver);

typedef int SDL_TimerID;
typedef Uint32 (*SDL_TimerCallback)(Uint32 interval, void* param);

SDL_TimerID SDL_AddTimer(Uint32 interval, SDL_TimerCallback callback, void* param);

Uint64 SDL_GetPerformanceCounter();

Uint64 SDL_GetPerformanceFrequency();

void SDL_Delay(Uint32 ms);

int SDL_SetPaletteColors(SDL_Palette* palette, const SDL_Color* colors, int firstcolor, int ncolors);

int SDL_HapticRumblePlay(SDL_Haptic* haptic, float strength, Uint32 length);

int SDL_JoystickRumble(SDL_Joystick* joystick, Uint16 lfr, Uint16 hfr, Uint32 duration);

int SDL_GameControllerRumble(SDL_GameController* controller, Uint16 lft, Uint16 hfr, Uint32 duration);

int SDL_NumJoysticks();

SDL_Joystick* SDL_JoystickOpen(int device_index);

#define SDL_GameControllerAddMappingsFromFile(file)   SDL_GameControllerAddMappingsFromRW(SDL_RWFromFile(file, "rb"), 1)

int SDL_GameControllerAddMappingsFromRW(SDL_RWops * rw, int freerw);

SDL_GameController* SDL_GameControllerOpen(int joystick_index);

SDL_bool SDL_IsGameController(int joystick_index);

const char* SDL_GetError();

void SDL_Quit();

SDL_Haptic* SDL_HapticOpen(int device_index);

int SDL_HapticRumbleInit(SDL_Haptic * haptic);

void SDL_SetTextInputRect(const SDL_Rect *rect);

void SDL_StartTextInput(void);

void SDL_StopTextInput(void);

#define SDL_KEYDOWN 1
#define SDL_KEYUP 2
#define SDL_USEREVENT 3
#define SDL_CONTROLLERAXISMOTION 4

typedef int SDL_Scancode;
typedef int SDL_Keycode;

typedef struct SDL_Keysym
{
    SDL_Scancode scancode;      /**< SDL physical key code - see SDL_Scancode for details */
    SDL_Keycode sym;            /**< SDL virtual key code - see SDL_Keycode for details */
    Uint16 mod;                 /**< current key modifiers - see SDL_Keymod for details */
    Uint32 unused;
} SDL_Keysym;

typedef struct SDL_KeyboardEvent
{
    Uint32 type;        /**< SDL_KEYDOWN or SDL_KEYUP */
    Uint32 timestamp;   /**< In milliseconds, populated using SDL_GetTicks() */
    Uint32 windowID;    /**< The window with keyboard focus, if any */
    Uint8 state;        /**< SDL_PRESSED or SDL_RELEASED */
    Uint8 repeat;       /**< Non-zero if this is a key repeat */
    Uint8 padding2;
    Uint8 padding3;
    SDL_Keysym keysym;  /**< The key that was pressed or released */
}
SDL_KeyboardEvent;


typedef int SDL_JoystickID;

typedef struct SDL_ControllerAxisEvent
{
    Uint32 type;        /**< SDL_CONTROLLERAXISMOTION */
    Uint32 timestamp;   /**< In milliseconds, populated using SDL_GetTicks() */
    SDL_JoystickID which; /**< The joystick instance id */
    Uint8 axis;         /**< The controller axis (SDL_GameControllerAxis) */
    Uint8 padding1;
    Uint8 padding2;
    Uint8 padding3;
    Sint16 value;       /**< The axis value (range: -32768 to 32767) */
    Uint16 padding4;
}
SDL_ControllerAxisEvent;

typedef struct
{
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    Sint32 code;
    void* data1;
    void* data2;
}
SDK_UserEvent;

typedef union
{
    Uint32 type;
    SDL_KeyboardEvent key;
    SDL_ControllerAxisEvent caxis;
    SDK_UserEvent user;
}
SDL_Event;

int SDL_PushEvent(SDL_Event * event);

int SDL_PollEvent(SDL_Event * event);

#define SDL_HINT_RENDER_SCALE_QUALITY ""
#define SDL_HINT_RENDER_VSYNC ""

SDL_bool SDL_SetHint(const char *name, const char *value);

#define SDL_INIT_TIMER 1
#define SDL_INIT_AUDIO 2
#define SDL_INIT_VIDEO 4
#define SDL_INIT_JOYSTICK 8
#define SDL_INIT_HAPTIC 16
#define SDL_INIT_GAMECONTROLLER 32
#define SDL_INIT_EVENTS 64
#define SDL_INIT_NOPARACHUTE 0
#define SDL_INIT_EVERYTHING (~0U)

int SDL_Init(Uint32 flags);

int SDL_InitSubSystem(Uint32 flags);

#define SDL_WINDOW_RESIZABLE 0
#define SDL_WINDOW_ALLOW_HIGHDPI 0
#define SDL_WINDOWPOS_UNDEFINED 0

SDL_Window * SDL_CreateWindow(const char *title, int x, int y, int w, int h, Uint32 flags);

void SDL_SetWindowIcon(SDL_Window * window, SDL_Surface * icon);

Uint32 SDL_GetWindowFlags(SDL_Window * window);

#define SDL_RENDERER_SOFTWARE 0
#define SDL_RENDERER_ACCELERATED 0
#define SDL_RENDERER_TARGETTEXTURE 0

SDL_Renderer * SDL_CreateRenderer(SDL_Window * window, int index, Uint32 flags);

typedef struct SDL_RendererInfo
{
    const char *name;           /**< The name of the renderer */
    Uint32 flags;               /**< Supported SDL_RendererFlags */
    Uint32 num_texture_formats; /**< The number of available texture formats */
    Uint32 texture_formats[16]; /**< The available texture formats */
    int max_texture_width;      /**< The maximum texture width */
    int max_texture_height;     /**< The maximum texture height */
}
SDL_RendererInfo;

int SDL_GetRendererInfo(SDL_Renderer * renderer, SDL_RendererInfo * info);

#define SDL_ENABLE 1
#define SDL_DISABLE 0

int SDL_ShowCursor(int toggle);

int SDL_SetRenderTarget(SDL_Renderer *renderer, SDL_Texture *texture);

int SDL_RenderClear(SDL_Renderer * renderer);

int SDL_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture, const SDL_Rect * srcrect, const SDL_Rect * dstrect);

int SDL_BlitScaled(const SDL_Surface *src, const SDL_Rect *srcrect, SDL_Surface *dst, const SDL_Rect *dstrect);

void SDL_RenderPresent(SDL_Renderer * renderer);

SDL_Surface* SDL_ConvertSurfaceFormat(SDL_Surface * src, Uint32 pixel_format, Uint32 flags);

#define SDL_KEYDOWN 1
