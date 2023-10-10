#include "core/def.h"
#include "core/utils.h"
#include "core/containers/common.h"

#include "core/platform.h"
#include "core/platform_sdl2.h"

#include "renderer/renderer.h"

#include "SDL.h"

using namespace core::utils;
using namespace renderer;

// ImGuiKey sdlk_to_imgui(SDL_Keycode keycode);

class Game {
public:
	int run()
	{
		core::utils::logger_init();

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
		auto renderer_state = renderer->create_state();
		if (!renderer_state) {
			LOG_ERROR("Failed to create renderer state");
			return EXIT_FAILURE;
		}
		auto time_last = platform_get_highresolution_time_seconds();
		auto texture = renderer_state->create_texture_from_file("C:/Users/roju2/OneDrive/Kuvat/duck.jpg");
		auto texture2 = renderer_state->create_texture_from_file("C:/Users/roju2/OneDrive/Kuvat/sloth.jpg");

		auto roboto_mono = renderer_state->create_font("C:/Users/roju2/p/foster/run_dir/fonts/RobotoMono-Regular.ttf", 18.f);

		u64 draw_calls_prev_frame = 0;
		String info_text;
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

				info_text = fmt::format("FPS: {}, Frame time: {}, draw_calls {}",
					fps, delta_time * 1.e3, draw_calls_prev_frame);
			}

			renderer_state->begin_frame({ 0.f, 0.f, 0.f, 1.f });

			renderer_state->draw_text(roboto_mono, (u8*)info_text.c_str(), info_text.size(), 10, 10);

			{
				Rect src = { 0 };
				src.w = (f32)texture->width;
				src.h = (f32)texture->height;
				Rect dst = { 0 };
				dst.x = 100.f;
				dst.y = 100.f;
				dst.w = 256.f;
				dst.h = 256.f;
				renderer_state->draw_sprite(texture, src, dst);
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
				renderer_state->draw_sprite(texture2, src, dst);
			}

			{
				ColorGradient gradient = {};
				gradient.top_left = {1,1,0,1};
				gradient.top_right = {0,1,0,1};
				gradient.bottom_left = {0,0,0,1};
				gradient.bottom_right = { 1,0,0,1 };
				Rect dst = { 0 };

				Rect src = { 0 };
				src.w = (f32)texture2->width;
				src.h = (f32)texture2->height;
				dst.x = 114.f;
				dst.y = 526.f;
				dst.w = 100.f;
				dst.h = 100.f;
				renderer_state->draw_sprite(texture2, src, dst);

				dst.x = 100.f;
				dst.y = 512.f;
				dst.w = 128.f;
				dst.h = 128.f;
				f32 edge_softness = 3.f;
				f32 border_radius = 16.f;
				f32 border_thickness = 16.f;
				renderer_state->draw_rect_pro(dst, edge_softness, border_radius, border_thickness, gradient);
			}
			renderer_state->end_frame(&draw_calls_prev_frame);
		}
		return EXIT_SUCCESS;
	}

	Pair<int, int> get_window_size()
	{
		Pair<int, int> size{};
		SDL_GetWindowSize(m_window, &size.first, &size.second);
		return size;
	}

private:
	SDL_Window* m_window{ nullptr };
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

#if 0
#include "def.h"
#include "platform.h"
#include "renderer.h"
#include "utils.h"

#include "utils.c"

struct Game
{
	RendererState* renderer_state;

	TextureHandle texture;
	TextureHandle texture2;
	FontHandle roboto_mono;

	f64 time_last;
	f64 last_frame_test;
	u32 frame_count;

	char info_text[1024];
	s32 info_text_size;

	u64 draw_calls_prev_frame;
};

#define GAME_MEMORY_SIZE (256*MiB)

DLL_EXPORT UPDATE_AND_RENDER(update_and_render)
{
	
}

#endif