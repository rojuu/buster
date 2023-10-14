#include "core/def.h"
#include "core/utils.h"
#include "core/containers/common.h"
#include "core/string_builder.h"
#include "core/asefile.h"

#include "core/platform.h"
#include "core/platform_sdl2.h"

#include "renderer/renderer.h"

#include "SDL.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"

using namespace bstr::core;
using namespace bstr::core::platform;
using namespace bstr::renderer;

namespace bstr {

class Game {
public:
	int run()
	{
		if (!set_cwd_to_run_dir()) {
			LOG_ERROR("Could not find run_dir");
			return EXIT_FAILURE;
		}

		core::logger_init();

		auto ase_file = AseFile::load_from_file("images/guy.ase");

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

		sdl2::set_window_handle(m_window);

		if (!m_window) {
			LOG_CRITICAL("Unable to create SDL_Window: {}", SDL_GetError());
			return EXIT_FAILURE;
		}
		defer {
			SDL_DestroyWindow(m_window);
		};

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
		//io.ConfigViewportsNoAutoMerge = true;
		//io.ConfigViewportsNoTaskBarIcon = true;
		io.IniFilename = nullptr;

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		ImGui_ImplSDL2_InitForD3D(m_window);

		auto renderer = create_renderer();
		if (!renderer) {
			LOG_ERROR("Failed to create renderer");
			return EXIT_FAILURE;
		}

		auto time_last = get_highresolution_time_seconds();
		auto texture = renderer->create_texture_from_file("images/duck.jpg");
		auto texture2 = renderer->create_texture_from_file("images/sloth.jpg");

		auto roboto_mono = renderer->create_font("fonts/RobotoMono-Regular.ttf", 18.f);

		bool show_demo_window = true;
		bool show_another_window = false;
		ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

		u64 draw_calls_prev_frame = 0;
		string info_text;
		f64 last_frame_test = 0;
		usz frame_count = 0;
		bool quit = false;
		while (!quit) {
			SDL_Event event;
			while (SDL_PollEvent(&event) != 0)
			{
				ImGui_ImplSDL2_ProcessEvent(&event);

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

			f64 time_now = get_highresolution_time_seconds();
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

			renderer->begin_frame({ clear_color.x, clear_color.y, clear_color.z, clear_color.w });
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();

			renderer->draw_text(roboto_mono, "what is going on", 30, 30);
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
	
			ImGui::Render();

			bool use_vsync = false;
			renderer->end_frame(use_vsync, &draw_calls_prev_frame);

			// Update and Render additional Platform Windows
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
			}
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
		auto cwd = get_current_working_directory();
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
			if (file_exists(run_dir_buf.c_str()))
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
			set_current_working_directory(run_dir_buf.c_str());
		}

		return run_dir_valid;
	}


private:
	SDL_Window* m_window{};
};

}

int main(int argc, char **argv)
{
	bstr::Game game;
	int result = game.run();
	return result;
}
