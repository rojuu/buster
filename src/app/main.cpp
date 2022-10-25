#include "core/def.h"
#include "core/scopeguard.h"
#include "core/utils.h"
#include "core/IPlatform.h"

#include "renderer/renderer.h"

#include "SDL.h"
#include "SDL_syswm.h"

#include "imgui.h"

class App : public IPlatform {
public:
	int run()
	{
		logger_init();

		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
			LOG_CRITICAL("Unable to initialize SDL: {}", SDL_GetError());
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
			LOG_CRITICAL("Unable to create SDL_Window: {}", SDL_GetError());
			return EXIT_FAILURE;
		}
		auto window_guard = ScopeGuard([this] {
			SDL_DestroyWindow(m_window);
		});

		zero_struct(&m_wm_info);
		SDL_VERSION(&m_wm_info.version);
		SDL_bool get_wminfo_result = SDL_GetWindowWMInfo(m_window, &m_wm_info);
		if (get_wminfo_result == SDL_FALSE) {
			LOG_CRITICAL("Unable to fetch SDL window info: {}", SDL_GetError());
			return EXIT_FAILURE;
		}

		Renderer renderer(*this);
		if (!renderer.init()) {
			return EXIT_FAILURE;
		}
		auto renderer_guard = ScopeGuard([&renderer] {
			renderer.deinit();
		});

		bool show_stats = false;

		bool quit = false;
		for (;;)
		{
			ImGuiIO& io = ImGui::GetIO();

			float scrollX = 0;
			float scrollY = 0;

			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				switch (event.type)
				{
					case SDL_QUIT:
					{
						quit = true;
					} break;

					case SDL_MOUSEWHEEL:
					{
						scrollX = event.wheel.preciseX;
						scrollY = event.wheel.preciseY;
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
							show_stats = !show_stats;
						} break;
						}
					} break;

					case SDL_TEXTINPUT:
					{
						for (char *ic = event.text.text; *ic; ic++)
						{
							io.AddInputCharacter((u32)*ic);
						}
					} break;
				}
			}
			if (quit) break;


			auto [window_width, window_height] = get_window_size();
			io.DisplaySize = ImVec2((float)window_width, (float)window_height);

			u64 freq = SDL_GetPerformanceFrequency();
			u64 counter = SDL_GetPerformanceCounter();

			const u64 now = counter;
			const u64 frameTime = now - m_last_time;
			m_last_time = now;
			io.DeltaTime = float(double(frameTime) / double(freq));

			s32 mouse_x, mouse_y;
			u32 mouse_state = SDL_GetMouseState(&mouse_x, &mouse_y);

			io.AddMousePosEvent((float)mouse_x, (float)mouse_y);
			io.AddMouseButtonEvent(ImGuiMouseButton_Left, (mouse_state & SDL_BUTTON_LMASK) != 0);
			io.AddMouseButtonEvent(ImGuiMouseButton_Right, (mouse_state & SDL_BUTTON_RMASK) != 0);
			io.AddMouseButtonEvent(ImGuiMouseButton_Middle, (mouse_state & SDL_BUTTON_MMASK) != 0);
			io.AddMouseWheelEvent(scrollX, scrollY);
#if 0
			io.ConfigFlags |= 0
				| ImGuiConfigFlags_NavEnableGamepad
				| ImGuiConfigFlags_NavEnableKeyboard
				;

			uint8_t modifiers = inputGetModifiersState();
			io.AddKeyEvent(ImGuiKey_ModShift, 0 != (modifiers & (entry::Modifier::LeftShift | entry::Modifier::RightShift)));
			io.AddKeyEvent(ImGuiKey_ModCtrl, 0 != (modifiers & (entry::Modifier::LeftCtrl | entry::Modifier::RightCtrl)));
			io.AddKeyEvent(ImGuiKey_ModAlt, 0 != (modifiers & (entry::Modifier::LeftAlt | entry::Modifier::RightAlt)));
			io.AddKeyEvent(ImGuiKey_ModSuper, 0 != (modifiers & (entry::Modifier::LeftMeta | entry::Modifier::RightMeta)));
			for (int32_t ii = 0; ii < (int32_t)entry::Key::Count; ++ii)
			{
				io.AddKeyEvent(m_keyMap[ii], inputGetKeyState(entry::Key::Enum(ii)));
				io.SetKeyEventNativeData(m_keyMap[ii], 0, 0, ii);
			}
#endif



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

			ImGui::NewFrame();
			//ImGuizmo::BeginFrame();

			renderer.begin_frame();

			renderer.dbg_print(0, 0, fmt::format("BUILD_TYPE: {}, IS_INTERNAL: {}, Press F1 to show rendering stats", build_type, internal_build));
			renderer.set_debug(show_stats ? Renderer::Debug_Stats : Renderer::Debug_Text);

			if (ImGui::Begin("Game")) {
				if (ImGui::Button("Win the game")) {
					LOG_DEBUG("You won!");
				}
				ImGui::End();
			}

			ImGui::Render();
			renderer.end_frame();
		}

		return EXIT_SUCCESS;
	}

	virtual WindowSize get_window_size() override
	{
		WindowSize size{};
		SDL_GetWindowSize(m_window, &size.width, &size.height);
		return size;
	}

	virtual void* get_native_window_handle() override { return m_wm_info.info.win.window; }
	virtual void* get_native_display_type() override { return nullptr; }

private:
	u64 m_last_time{ 0 };
	SDL_Window* m_window{ nullptr };
	SDL_SysWMinfo m_wm_info = { 0 };
};

int main(int argc, char **argv)
{
	App app;
	return app.run();
}
