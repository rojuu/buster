#include "core/def.h"
#include "core/utils.h"
#include "core/containers/common.h"

#include "SDL.h"
#include "SDL_syswm.h"

#include <bgfx/bgfx.h>

#include "imgui.h"
#include "imgui_bgfx.h"


ImGuiKey sdlk_to_imgui(SDL_Keycode keycode);

class App {
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

		//bgfx::renderFrame(); // Call before init to force rendering from same thread
		bgfx::Init init;
		init.platformData.ndt = get_native_display_type();
		init.platformData.nwh = get_native_window_handle();

#if IS_DEBUG_BUILD
		init.debug = true;
#endif
		{
			auto [window_width, window_height] = get_window_size();
			m_old_window_width = window_width;
			m_old_window_height = window_height;
			init.resolution.width = (uint32_t)window_width;
			init.resolution.height = (uint32_t)window_height;
			init.resolution.reset = BGFX_RESET_VSYNC;
		}

		if (!bgfx::init(init)) {
			return EXIT_FAILURE;
		}

		bgfx::setViewClear(s_clear_view, BGFX_CLEAR_COLOR);
		bgfx::setViewRect(s_clear_view, 0, 0, bgfx::BackbufferRatio::Equal);

		imguiCreate(s_imgui_view);

		auto bgfx_guard = ScopeGuard([] {
			imguiDestroy();
			bgfx::shutdown();
		});


		bool show_stats = false;

		bool quit = false;
		for (;;)
		{
			ImGuiIO& io = ImGui::GetIO();

			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

			io.ConfigFlags |= 0
				//| ImGuiConfigFlags_NavEnableGamepad
				| ImGuiConfigFlags_NavEnableKeyboard
				;

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

					case SDL_KEYDOWN:
					case SDL_KEYUP:
					{
						bool down = event.type == SDL_KEYDOWN;

						SDL_Keycode keycode = event.key.keysym.sym;

						ImGuiKey imgui_key = sdlk_to_imgui(keycode);
						if (imgui_key != ImGuiKey_None) {
							io.AddKeyEvent(imgui_key, down);
							io.SetKeyEventNativeData(imgui_key, keycode, event.key.keysym.scancode);
						}

						auto is_down = [&](auto modifier) -> bool {
							return down && event.key.keysym.sym == modifier;
						};

						io.AddKeyEvent(ImGuiKey_ModShift, is_down(SDLK_LSHIFT) || is_down(SDLK_RSHIFT));
						io.AddKeyEvent(ImGuiKey_ModCtrl, is_down(SDLK_LCTRL) || is_down(SDLK_RCTRL));
						io.AddKeyEvent(ImGuiKey_ModAlt, is_down(SDLK_LALT) || is_down(SDLK_RALT));
						io.AddKeyEvent(ImGuiKey_ModSuper, is_down(SDL_SCANCODE_LGUI) || is_down(SDL_SCANCODE_RGUI));

						if (is_down(SDLK_ESCAPE)) quit = true;
						if (is_down(SDLK_F1)) show_stats = !show_stats;
					} break;

					case SDL_MOUSEWHEEL:
					{
						scrollX = event.wheel.preciseX;
						scrollY = event.wheel.preciseY;
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

			ImGui::NewFrame();
			//ImGuizmo::BeginFrame();

			ImGui::DockSpaceOverViewport();

			if (ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("Exit", "Escape")) {
						quit = true;
					}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Debug"))
				{
					if (ImGui::MenuItem("Toggle stats", "F1")) {
						show_stats = !show_stats;
					}
					ImGui::EndMenu();
				}
			}
			ImGui::EndMainMenuBar();



			bgfx::dbgTextClear();
			if (window_width != m_old_window_width || window_height != m_old_window_height) {
				bgfx::reset((u32)window_width, (u32)window_height, BGFX_RESET_NONE);
				bgfx::setViewRect(s_clear_view, 0, 0, bgfx::BackbufferRatio::Equal);
			}

			m_old_window_width = window_width;
			m_old_window_height = window_height;

			bgfx::touch(s_clear_view);
			imguiBeginFrame(s_imgui_view);

#if 0
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
			bgfx::dbgTextPrintf(0, 1, 0x0f, "BUILD_TYPE: %s, IS_INTERNAL: %s, Press F1 to show rendering stats", build_type, internal_build);
			bgfx::setDebug(show_stats ? BGFX_DEBUG_STATS : BGFX_DEBUG_TEXT);
#endif

			if (ImGui::Begin("Game")) {
				if (ImGui::Button("Win the game")) {
					LOG_DEBUG("You won!");
				}
			}
			ImGui::End();

			ImGui::ShowDemoWindow();


			ImGui::Render();

			imguiEndFrame();
			bgfx::frame();
		}

		return EXIT_SUCCESS;
	}

	pair<int, int> get_window_size()
	{
		pair<int, int> size{};
		SDL_GetWindowSize(m_window, &size.first, &size.second);
		return size;
	}

	void* get_native_window_handle() { return m_wm_info.info.win.window; }
	void* get_native_display_type() { return nullptr; }

private:
	enum ViewIds : bgfx::ViewId
	{
		s_clear_view,
		s_imgui_view,
	};

	u64 m_last_time{ 0 };

	s32 m_old_window_width{ 0 };
	s32 m_old_window_height{ 0 };

	SDL_Window* m_window{ nullptr };
	SDL_SysWMinfo m_wm_info = { 0 };
};

int main(int argc, char **argv)
{
	App app;
	return app.run();
}


ImGuiKey sdlk_to_imgui(SDL_Keycode keycode)
{
	static hash_map<SDL_Keycode, ImGuiKey> map{
		{ SDLK_TAB, ImGuiKey_Tab },
		{ SDLK_LEFT, ImGuiKey_LeftArrow },
		{ SDLK_RIGHT, ImGuiKey_RightArrow },
		{ SDLK_UP, ImGuiKey_UpArrow },
		{ SDLK_DOWN, ImGuiKey_DownArrow },
		{ SDLK_PAGEUP, ImGuiKey_PageUp },
		{ SDLK_PAGEDOWN, ImGuiKey_PageDown },
		{ SDLK_HOME, ImGuiKey_Home },
		{ SDLK_END, ImGuiKey_End },
		{ SDLK_INSERT, ImGuiKey_Insert },
		{ SDLK_DELETE, ImGuiKey_Delete },
		{ SDLK_BACKSPACE, ImGuiKey_Backspace },
		{ SDLK_SPACE, ImGuiKey_Space },
		{ SDLK_RETURN, ImGuiKey_Enter },
		{ SDLK_ESCAPE, ImGuiKey_Escape },
		{ SDLK_MENU, ImGuiKey_Menu },
		{ SDLK_0, ImGuiKey_0 },
		{ SDLK_1, ImGuiKey_1 },
		{ SDLK_2, ImGuiKey_2 },
		{ SDLK_3, ImGuiKey_3 },
		{ SDLK_4, ImGuiKey_4 },
		{ SDLK_5, ImGuiKey_5 },
		{ SDLK_6, ImGuiKey_6 },
		{ SDLK_7, ImGuiKey_7 },
		{ SDLK_8, ImGuiKey_8 },
		{ SDLK_9, ImGuiKey_9 },
		{ 'a', ImGuiKey_A },
		{ 'b', ImGuiKey_B },
		{ 'c', ImGuiKey_C },
		{ 'd', ImGuiKey_D },
		{ 'e', ImGuiKey_E },
		{ 'f', ImGuiKey_F },
		{ 'g', ImGuiKey_G },
		{ 'h', ImGuiKey_H },
		{ 'i', ImGuiKey_I },
		{ 'j', ImGuiKey_J },
		{ 'k', ImGuiKey_K },
		{ 'l', ImGuiKey_L },
		{ 'm', ImGuiKey_M },
		{ 'n', ImGuiKey_N },
		{ 'o', ImGuiKey_O },
		{ 'p', ImGuiKey_P },
		{ 'q', ImGuiKey_Q },
		{ 'r', ImGuiKey_R },
		{ 's', ImGuiKey_S },
		{ 't', ImGuiKey_T },
		{ 'u', ImGuiKey_U },
		{ 'v', ImGuiKey_V },
		{ 'w', ImGuiKey_W },
		{ 'x', ImGuiKey_X},
		{ 'y', ImGuiKey_Y, },
		{ 'z', ImGuiKey_Z},
		{ SDLK_F1, ImGuiKey_F1, },
		{ SDLK_F2, ImGuiKey_F2, },
		{ SDLK_F3, ImGuiKey_F3, },
		{ SDLK_F4, ImGuiKey_F4, },
		{ SDLK_F5, ImGuiKey_F5, },
		{ SDLK_F6, ImGuiKey_F6 },
		{ SDLK_F7, ImGuiKey_F7, },
		{ SDLK_F8, ImGuiKey_F8, },
		{ SDLK_F9, ImGuiKey_F9, },
		{ SDLK_F10, ImGuiKey_F10, },
		{ SDLK_F11, ImGuiKey_F11, },
		{ SDLK_F12, ImGuiKey_F12 },
		{ '\'', ImGuiKey_Apostrophe},        // '
		{ ',', ImGuiKey_Comma},             //  
		{ '-', ImGuiKey_Minus},             // -
		{ '.', ImGuiKey_Period},            // .
		{ '/', ImGuiKey_Slash},             // /
		{ ';', ImGuiKey_Semicolon},         // ;
		{ '=', ImGuiKey_Equal},             // =
		{ '[', ImGuiKey_LeftBracket},       // [
		{ '\\', ImGuiKey_Backslash},         // \ (this text inhibit multiline comment caused by backslash)
		{ ']', ImGuiKey_RightBracket},      // ]
		{ '`', ImGuiKey_GraveAccent},       // `
		{ SDLK_CAPSLOCK, ImGuiKey_CapsLock },
		{ SDLK_SCROLLLOCK, ImGuiKey_ScrollLock },
		{ SDLK_NUMLOCKCLEAR, ImGuiKey_NumLock },
		{ SDLK_PRINTSCREEN, ImGuiKey_PrintScreen },
		{ SDLK_PAUSE, ImGuiKey_Pause },
		{ SDLK_KP_0, ImGuiKey_Keypad0 },
		{ SDLK_KP_1, ImGuiKey_Keypad1 },
		{ SDLK_KP_2, ImGuiKey_Keypad2 },
		{ SDLK_KP_3, ImGuiKey_Keypad3 },
		{ SDLK_KP_4, ImGuiKey_Keypad4 },
		{ SDLK_KP_5, ImGuiKey_Keypad5 },
		{ SDLK_KP_6, ImGuiKey_Keypad6 },
		{ SDLK_KP_7, ImGuiKey_Keypad7 },
		{ SDLK_KP_8, ImGuiKey_Keypad8 },
		{ SDLK_KP_9, ImGuiKey_Keypad9 },
		{ SDLK_KP_DECIMAL, ImGuiKey_KeypadDecimal },
		{ SDLK_KP_DIVIDE, ImGuiKey_KeypadDivide },
		{ SDLK_KP_MULTIPLY, ImGuiKey_KeypadMultiply },
		{ SDLK_KP_MINUS, ImGuiKey_KeypadSubtract },
		{ SDLK_KP_PLUS, ImGuiKey_KeypadAdd },
		{ SDLK_KP_ENTER, ImGuiKey_KeypadEnter },
		{ SDLK_KP_EQUALS, ImGuiKey_KeypadEqual },
	};
#if 0
	{, ImGuiKey_GamepadStart },          // Menu (Xbox)      + (Switch)   Start/Options (PS)
	{ , ImGuiKey_GamepadBack },           // View (Xbox)      - (Switch)   Share (PS)
	{ , ImGuiKey_GamepadFaceLeft },       // X (Xbox)         Y (Switch)   Square (PS)        // Tap: Toggle Menu. Hold: Windowing mode (Focus/Move/Resize windows)
	{ , ImGuiKey_GamepadFaceRight },      // B (Xbox)         A (Switch)   Circle (PS)        // Cancel / Close / Exit
	{ , ImGuiKey_GamepadFaceUp },         // Y (Xbox)         X (Switch)   Triangle (PS)      // Text Input / On-screen Keyboard
	{ , ImGuiKey_GamepadFaceDown },       // A (Xbox)         B (Switch)   Cross (PS)         // Activate / Open / Toggle / Tweak
	{ , ImGuiKey_GamepadDpadLeft },       // D-pad Left                                       // Move / Tweak / Resize Window (in Windowing mode)
	{ , ImGuiKey_GamepadDpadRight },      // D-pad Right                                      // Move / Tweak / Resize Window (in Windowing mode)
	{ , ImGuiKey_GamepadDpadUp },         // D-pad Up                                         // Move / Tweak / Resize Window (in Windowing mode)
	{ , ImGuiKey_GamepadDpadDown },       // D-pad Down                                       // Move / Tweak / Resize Window (in Windowing mode)
	{ , ImGuiKey_GamepadL1 },             // L Bumper (Xbox)  L (Switch)   L1 (PS)            // Tweak Slower / Focus Previous (in Windowing mode)
	{ , ImGuiKey_GamepadR1 },             // R Bumper (Xbox)  R (Switch)   R1 (PS)            // Tweak Faster / Focus Next (in Windowing mode)
	{ , ImGuiKey_GamepadL2 },             // L Trig. (Xbox)   ZL (Switch)  L2 (PS) [Analog]
	{ , ImGuiKey_GamepadR2 },             // R Trig. (Xbox)   ZR (Switch)  R2 (PS) [Analog]
	{ , ImGuiKey_GamepadL3 },             // L Stick (Xbox)   L3 (Switch)  L3 (PS)
	{ , ImGuiKey_GamepadR3 },             // R Stick (Xbox)   R3 (Switch)  R3 (PS)
	{ , ImGuiKey_GamepadLStickLeft },     // [Analog]                                         // Move Window (in Windowing mode)
	{ , ImGuiKey_GamepadLStickRight },    // [Analog]                                         // Move Window (in Windowing mode)
	{ , ImGuiKey_GamepadLStickUp },       // [Analog]                                         // Move Window (in Windowing mode)
	{ , ImGuiKey_GamepadLStickDown },     // [Analog]                                         // Move Window (in Windowing mode)
	{ , ImGuiKey_GamepadRStickLeft },     // [Analog]
	{ , ImGuiKey_GamepadRStickRight },    // [Analog]
	{ , ImGuiKey_GamepadRStickUp },       // [Analog]
	{ , ImGuiKey_GamepadRStickDown },     // [Analog]
#endif
		auto it = map.find(keycode);
	if (it != map.end()) {
		return it->second;
	}
	return ImGuiKey_None;
}
