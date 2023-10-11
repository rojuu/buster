#include "core/def.h"
#include "core/utils.h"
#include "core/containers/common.h"
#include "core/string_builder.h"

#include "core/platform.h"
#include "core/platform_sdl2.h"

#include "renderer/renderer.h"

#include "SDL.h"

using namespace core;
using namespace renderer;

// ImGuiKey sdlk_to_imgui(SDL_Keycode keycode);

class Game {
public:
	int run()
	{
		if (!set_cwd_to_run_dir()) {
			LOG_ERROR("Could not find run_dir");
			return EXIT_FAILURE;
		}

		core::logger_init();

		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
			LOG_CRITICAL("Unable to initialize SDL: {}", SDL_GetError());
			return EXIT_FAILURE;
		}
		defer {
			SDL_Quit();
		};

		m_window = SDL_CreateWindow(
			"A window title",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			1600, 900,
			SDL_WINDOW_RESIZABLE);

		sdl2_set_window_handle(m_window);

		if (!m_window) {
			LOG_CRITICAL("Unable to create SDL_Window: {}", SDL_GetError());
			return EXIT_FAILURE;
		}
		defer {
			SDL_DestroyWindow(m_window);
		};

		auto renderer = create_renderer();
		if (!renderer) {
			LOG_ERROR("Failed to create renderer");
			return EXIT_FAILURE;
		}
		auto time_last = platform_get_highresolution_time_seconds();
		auto texture = renderer->create_texture_from_file("images/duck.jpg");
		auto texture2 = renderer->create_texture_from_file("images/sloth.jpg");

		auto roboto_mono = renderer->create_font("fonts/RobotoMono-Regular.ttf", 18.f);

		u64 draw_calls_prev_frame = 0;
		string info_text_str;
		f64 last_frame_test = 0;
		usz frame_count = 0;
		bool quit = false;
		while (!quit) {
			SDL_Event event;
			while (SDL_PollEvent(&event) != 0)
			{
				if (event.type == SDL_QUIT)
				{
					quit = true;
				}
				else if (event.type == SDL_KEYDOWN)
				{
					if (event.key.keysym.sym == SDLK_ESCAPE)
					{
						quit = true;
					}
				}
			}

			f64 time_now = platform_get_highresolution_time_seconds();
			f32 delta_time = (f32)(time_now - time_last);
			time_last = time_now;

			++frame_count;
			f64 frame_test_delta = time_now - last_frame_test;
			if (frame_test_delta >= 0.5)
			{
				u32 fps = (u32)((f64)(frame_count / frame_test_delta));
				frame_count = 0;
				last_frame_test = time_now;

				info_text_str = fmt::format("FPS: {}, Frame time: {}, draw_calls {}",
					fps, delta_time * 1.e3, draw_calls_prev_frame);
			}

			renderer->begin_frame({ 0.f, 0.f, 0.f, 1.f });

			span<u8> info_text{ (u8*)info_text_str.data(), info_text_str.size() };
			renderer->draw_text(roboto_mono, info_text, 10, 10);

			{
				Rect src = { 0 };
				src.w = (f32)texture->width;
				src.h = (f32)texture->height;
				Rect dst = { 0 };
				dst.x = 100.f;
				dst.y = 100.f;
				dst.w = 256.f;
				dst.h = 256.f;
				renderer->draw_sprite(texture, src, dst);
			}

			{
				Rect src = { 0 };
				src.w = (f32)texture2->width;
				src.h = (f32)texture2->height;
				Rect dst = { 0 };
				dst.x = 512.f;
				dst.y = 100.f;
				dst.w = 256.f;
				dst.h = 256.f;
				renderer->draw_sprite(texture2, src, dst);
			}

			bool use_vsync = false;
			renderer->end_frame(use_vsync, &draw_calls_prev_frame);
		}
		return EXIT_SUCCESS;
	}

	pair<int, int> get_window_size()
	{
		pair<int, int> size{};
		SDL_GetWindowSize(m_window, &size.first, &size.second);
		return size;
	}


	bool set_cwd_to_run_dir()
	{
		bool run_dir_valid = false;
		auto cwd = platform_get_current_working_directory();
		if (cwd.empty()) {
			return false;
		}

		string_builder run_dir_buf;
		run_dir_buf.reserve(4);
		run_dir_buf.append(cwd);
		if (run_dir_buf[run_dir_buf.size() - 1] != PATH_DELIMITER) {
			run_dir_buf.append(PATH_DELIMITER);
		}
		auto run_dir_postfix = string("run_dir") + PATH_DELIMITER;
		for (usz iterations = 0; iterations < 30; ++iterations) 
		{
			run_dir_buf.append(run_dir_postfix);
			if (platform_file_exists(run_dir_buf.c_str()))
			{
				run_dir_valid = true;
				break;
			}
			else
			{
				run_dir_buf.resize(run_dir_buf.size() - run_dir_postfix.size());
				for (int i = (int)run_dir_buf.size() - 2; i >= 0; --i) {
					run_dir_buf.resize(i+1);
					if (run_dir_buf[i] == PATH_DELIMITER) {
						break;
					}
				}
			}
		}
		if (run_dir_valid) {
			platform_set_current_working_directory(run_dir_buf.c_str());
		}

		return run_dir_valid;
	}


private:
	SDL_Window* m_window{};
};

int main(int argc, char **argv)
{
	Game game;
	int result = game.run();
	return result;
}

#if 0
ImGuiKey sdlk_to_imgui(SDL_Keycode keycode)
{
	static HashMap<SDL_Keycode, ImGuiKey> map{
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
#endif
