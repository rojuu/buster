#include "def.h"
#include "scopeguard.h"
#include "utils.h"

#include <bx/bx.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include "SDL.h"
#include "SDL_syswm.h"

class App {
public:
	int run()
	{
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
			LOG(FATAL, "Unable to initialize SDL: {}", SDL_GetError());
			return EXIT_FAILURE;
		}
		auto sdl_guard = ScopeGuard([] {
			SDL_Quit();
			});

		m_window = SDL_CreateWindow(
			"A window title",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			1024, 768,
			SDL_WINDOW_RESIZABLE);
		if (!m_window) {
			LOG(FATAL, "Unable to create SDL_Window: {}", SDL_GetError());
			return EXIT_FAILURE;
		}
		auto window_guard = ScopeGuard([this] {
			SDL_DestroyWindow(m_window);
			});

		//bgfx::renderFrame(); // Call before init to force rendering from same thread
		bgfx::Init init;
		SDL_SysWMinfo wm_info = { 0 };
		SDL_VERSION(&wm_info.version);
		SDL_bool get_wminfo_result = SDL_GetWindowWMInfo(m_window, &wm_info);
		ASSERT_UNCHECKED(get_wminfo_result, "");

#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
#error "Unsupported platform"
		//init.platformData.ndt = glfwGetX11Display();
		//init.platformData.nwh = (void*)(uintptr_t)glfwGetX11Window(window);
#elif BX_PLATFORM_OSX
#error "Unsupported platform"
		//init.platformData.nwh = glfwGetCocoaWindow(window);
#elif BX_PLATFORM_WINDOWS
		init.platformData.nwh = wm_info.info.win.window;
#endif

		SDL_GetWindowSize(m_window, &m_window_width, &m_window_height);
		m_old_window_width = m_window_width;
		m_old_window_height = m_window_height;
		init.resolution.width = (uint32_t)m_window_width;
		init.resolution.height = (uint32_t)m_window_height;
		init.resolution.reset = BGFX_RESET_VSYNC;

		if (!bgfx::init(init)) {
			return 1;
		}
		auto bgfx_guard = ScopeGuard([] {
			bgfx::shutdown();
		});

		bgfx::setViewClear(clear_view, BGFX_CLEAR_COLOR);
		bgfx::setViewRect(clear_view, 0, 0, bgfx::BackbufferRatio::Equal);

		bool quit = false;
		for (;;)
		{
			m_old_window_width = m_window_width;
			m_old_window_height = m_window_height;
			SDL_GetWindowSize(m_window, &m_window_width, &m_window_height);

			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				switch (event.type)
				{
					case SDL_QUIT:
					{
						quit = true;
					} break;

					case SDL_KEYDOWN:
					{
						switch (event.key.keysym.sym)
						{
						case SDLK_ESCAPE:
						{
							quit = true;
						} break;

						case SDLK_F1:
						{
							m_show_stats = !m_show_stats;
						} break;
						}
					} break;
				}
			}
			if (quit) break;

			render();
		}

		return EXIT_SUCCESS;
	}


	void render()
	{
		if (m_window_width != m_old_window_width || m_window_height != m_old_window_height) {
			bgfx::reset((u32)m_window_width, (u32)m_window_height, BGFX_RESET_VSYNC);
			bgfx::setViewRect(clear_view, 0, 0, bgfx::BackbufferRatio::Equal);
		}
		// This dummy draw call is here to make sure that view 0 is cleared if no other draw calls are submitted to view 0.
		bgfx::touch(clear_view);

		bgfx::dbgTextClear();
		const bgfx::Stats* stats = bgfx::getStats();
#if IS_INTERNAL_BUILD
		const char* internal_build = "YES";
#elif IS_INTERNAL_BUILD == 0
		const char* internal_build = "NO";
#else
		const char* internal_build = "UNKNOWN";
#endif
#if IS_DEBUG_BUILD
		const char* build_type = "DEBUG";
#elif IS_RELEASE_BUILD
		const char* build_type = "RELEASE";
#else
		const char* build_type = "UNKNOWN";
#endif
		bgfx::dbgTextPrintf(0, 0, 0x0f, "Drawcalls: %d, BUILD_TYPE: %s, IS_INTERNAL: %s, Press F1 to toggle detailed rendering stats",  stats->numDraw, build_type, internal_build);
		bgfx::setDebug(m_show_stats ? BGFX_DEBUG_STATS : BGFX_DEBUG_TEXT);

		bgfx::frame();
	}

private:
	static constexpr bgfx::ViewId clear_view = 0;

	SDL_Window* m_window{ nullptr };
	s32 m_window_width{ 0 };
	s32 m_window_height{ 0 };
	s32 m_old_window_width{ 0 };
	s32 m_old_window_height{ 0 };
	bool m_show_stats{false};
};

int main(int argc, char **argv)
{
	App app;
	return app.run();
}
