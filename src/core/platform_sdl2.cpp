#include "platform.h"
#include "utils.h"

#include "SDL.h"
#include "SDL_syswm.h"

#include "platform_sdl2.h"

namespace bstr::core::platform {

static SDL_Window* g_window_handle = 0;

void sdl2::set_window_handle(SDL_Window *window)
{
    g_window_handle = window;
}

WindowHandle get_native_window_handle()
{
    WindowHandle result = {};
    
    SDL_SysWMinfo wm_info = {};
    zero_struct(&wm_info);
    SDL_VERSION(&wm_info.version);
    SDL_bool get_wminfo_result = SDL_GetWindowWMInfo(g_window_handle, &wm_info);
    if (get_wminfo_result != SDL_FALSE) {
        result.nwh = wm_info.info.win.window;
    }
    else {
        LOG_CRITICAL("Unable to fetch SDL window info: {}", SDL_GetError());
    }

    return result;
}

f64 get_highresolution_time_seconds()
{
    f64 result = 0;

    Uint64 counter = SDL_GetPerformanceCounter();
    Uint64 freq = SDL_GetPerformanceFrequency();

    result = (f64)counter / (f64)freq;

    return result;
}

WindowSize get_window_size()
{
    WindowSize size = { 0 };
    SDL_GetWindowSize(g_window_handle, &size.width, &size.height);
    return size;
}

}