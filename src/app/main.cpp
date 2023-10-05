#include "core/def.h"
#include "core/utils.h"
#include "core/containers/common.h"

#include "bgfx_callback.h"

#include "SDL.h"
#include "SDL_syswm.h"

#include <bx/bx.h>
#include <bx/math.h>
#include <bgfx/bgfx.h>

#include "imgui.h"
#include "imgui_bgfx.h"


struct PosColorVertex
{
	float m_x;
	float m_y;
	float m_z;
	uint32_t m_abgr;

	static void init()
	{
		ms_layout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
			.end();
	};

	static bgfx::VertexLayout ms_layout;
};

bgfx::VertexLayout PosColorVertex::ms_layout;

static PosColorVertex s_cubeVertices[] =
{
	{-1.0f,  1.0f,  1.0f, 0xff000000 },
	{ 1.0f,  1.0f,  1.0f, 0xff0000ff },
	{-1.0f, -1.0f,  1.0f, 0xff00ff00 },
	{ 1.0f, -1.0f,  1.0f, 0xff00ffff },
	{-1.0f,  1.0f, -1.0f, 0xffff0000 },
	{ 1.0f,  1.0f, -1.0f, 0xffff00ff },
	{-1.0f, -1.0f, -1.0f, 0xffffff00 },
	{ 1.0f, -1.0f, -1.0f, 0xffffffff },
};

static const uint16_t s_cubeTriList[] =
{
	0, 1, 2, // 0
	1, 3, 2,
	4, 6, 5, // 2
	5, 6, 7,
	0, 2, 4, // 4
	4, 2, 6,
	1, 5, 3, // 6
	5, 7, 3,
	0, 4, 1, // 8
	4, 5, 1,
	2, 3, 6, // 10
	6, 3, 7,
};

static const uint16_t s_cubeTriStrip[] =
{
	0, 1, 2,
	3,
	7,
	1,
	5,
	0,
	4,
	2,
	6,
	7,
	4,
	5,
};

static const uint16_t s_cubeLineList[] =
{
	0, 1,
	0, 2,
	0, 4,
	1, 3,
	1, 5,
	2, 3,
	2, 6,
	3, 7,
	4, 5,
	4, 6,
	5, 7,
	6, 7,
};

static const uint16_t s_cubeLineStrip[] =
{
	0, 2, 3, 1, 5, 7, 6, 4,
	0, 2, 6, 4, 5, 7, 3, 1,
	0,
};

static const uint16_t s_cubePoints[] =
{
	0, 1, 2, 3, 4, 5, 6, 7
};

static const char* s_ptNames[]
{
	"Triangle List",
	"Triangle Strip",
	"Lines",
	"Line Strip",
	"Points",
};

static const uint64_t s_ptState[]
{
	UINT64_C(0),
	BGFX_STATE_PT_TRISTRIP,
	BGFX_STATE_PT_LINES,
	BGFX_STATE_PT_LINESTRIP,
	BGFX_STATE_PT_POINTS,
};
static_assert(ARRAY_COUNT(s_ptState) == ARRAY_COUNT(s_ptNames));




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
			1600, 900,
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
		init.callback = &bgfx_callbacks;
		init.profile = true;

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


		bgfx::setViewClear(s_clear_view, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH);
		for (bgfx::ViewId id = 0; id < s_view_count; ++id) {
			bgfx::setViewRect(id, 0, 0, bgfx::BackbufferRatio::Equal);
		}

		bool use_imgui = true;
		imguiCreate(s_imgui_view);

		auto bgfx_guard = ScopeGuard([] {
			imguiDestroy();
			bgfx::shutdown();
		});

		create_cube_stuff();
		auto cube_guard = ScopeGuard([this] {
			destroy_cube_stuff();
		});

		ImGuiIO& io = ImGui::GetIO();

		//io.IniFilename = NULL;

		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		io.ConfigFlags |= 0
			//| ImGuiConfigFlags_NavEnableGamepad
			| ImGuiConfigFlags_NavEnableKeyboard
			;
		
		bool show_stats = true;
		bool quit = false;
		for (;;)
		{

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
						auto is_down = [&](auto modifier) -> bool {
							return down && event.key.keysym.sym == modifier;
						};

						if (use_imgui) {
							ImGuiKey imgui_key = sdlk_to_imgui(keycode);
							if (imgui_key != ImGuiKey_None) {
								io.AddKeyEvent(imgui_key, down);
								io.SetKeyEventNativeData(imgui_key, keycode, event.key.keysym.scancode);
							}

							io.AddKeyEvent(ImGuiKey_ModShift, is_down(SDLK_LSHIFT) || is_down(SDLK_RSHIFT));
							io.AddKeyEvent(ImGuiKey_ModCtrl, is_down(SDLK_LCTRL) || is_down(SDLK_RCTRL));
							io.AddKeyEvent(ImGuiKey_ModAlt, is_down(SDLK_LALT) || is_down(SDLK_RALT));
							io.AddKeyEvent(ImGuiKey_ModSuper, is_down(SDL_SCANCODE_LGUI) || is_down(SDL_SCANCODE_RGUI));
						}

						if (is_down(SDLK_ESCAPE)) quit = true;
						if (is_down(SDLK_F1)) show_stats = !show_stats;
						if (is_down(SDLK_F2)) use_imgui = !use_imgui;
					} break;

					case SDL_MOUSEWHEEL:
					{
						scrollX = event.wheel.preciseX;
						scrollY = event.wheel.preciseY;
					} break;

					case SDL_TEXTINPUT:
					{
						if (use_imgui) {
							for (char* ic = event.text.text; *ic; ic++)
							{
								io.AddInputCharacter((u32)*ic);
							}
						}
					} break;
				}
			}
			if (quit) break;
			
			if (use_imgui) imguiBeginFrame(s_imgui_view);

			auto [window_width, window_height] = get_window_size();
			io.DisplaySize = ImVec2((float)window_width, (float)window_height);

			u64 freq = SDL_GetPerformanceFrequency();
			u64 counter = SDL_GetPerformanceCounter();

			const u64 now = counter;
			const u64 frameTime = now - m_last_time;
			m_last_time = now;
			io.DeltaTime = float(double(frameTime) / double(freq));
			
			if (use_imgui) {
				s32 mouse_x, mouse_y;
				u32 mouse_state = SDL_GetMouseState(&mouse_x, &mouse_y);

				io.AddMousePosEvent((float)mouse_x, (float)mouse_y);
				io.AddMouseButtonEvent(ImGuiMouseButton_Left, (mouse_state & SDL_BUTTON_LMASK) != 0);
				io.AddMouseButtonEvent(ImGuiMouseButton_Right, (mouse_state & SDL_BUTTON_RMASK) != 0);
				io.AddMouseButtonEvent(ImGuiMouseButton_Middle, (mouse_state & SDL_BUTTON_MMASK) != 0);
				io.AddMouseWheelEvent(scrollX, scrollY);
			}

			ImGui::NewFrame();

			bgfx::dbgTextClear();
			if (window_width != m_old_window_width || window_height != m_old_window_height) {
				bgfx::reset((u32)window_width, (u32)window_height, BGFX_RESET_NONE);

				for (bgfx::ViewId id = 0; id < s_view_count; ++id) {
					bgfx::setViewRect(id, 0, 0, bgfx::BackbufferRatio::Equal);
				}
			}

			m_old_window_width = window_width;
			m_old_window_height = window_height;

			bgfx::touch(s_clear_view);

			render_cube_stuff();

			bgfx::setDebug(show_stats ? BGFX_DEBUG_STATS : 0);

			if (ImGui::Begin("Game")) {
				if (ImGui::Button("Win the game")) {
					LOG_DEBUG("You won!");
				}
			}
			ImGui::End();

			ImGui::ShowDemoWindow();

			ImGui::Render();

			if (use_imgui) imguiEndFrame();
			bgfx::frame();
		}

		return EXIT_SUCCESS;
	}

	Pair<int, int> get_window_size()
	{
		Pair<int, int> size{};
		SDL_GetWindowSize(m_window, &size.first, &size.second);
		return size;
	}

	void destroy_cube_stuff()
	{
		// Cleanup.
		for (uint32_t ii = 0; ii < ARRAY_COUNT(m_ibh); ++ii)
		{
			bgfx::destroy(m_ibh[ii]);
		}
		bgfx::destroy(m_vbh);
		bgfx::destroy(m_program);
	}

	void render_cube_stuff()
	{
		auto [width, height] = get_window_size();


		ImGui::SetNextWindowPos(
			ImVec2(width - width / 5.0f - 10.0f, 10.0f)
			, ImGuiCond_FirstUseEver
		);
		ImGui::SetNextWindowSize(
			ImVec2(width / 5.0f, height / 3.5f)
			, ImGuiCond_FirstUseEver
		);
		ImGui::Begin("Settings"
			, NULL
			, 0
		);

		ImGui::Checkbox("Write R", &m_r);
		ImGui::Checkbox("Write G", &m_g);
		ImGui::Checkbox("Write B", &m_b);
		ImGui::Checkbox("Write A", &m_a);

		ImGui::Text("Primitive topology:");
		ImGui::Combo("##topology", (int*)&m_pt, s_ptNames, BX_COUNTOF(s_ptNames));


		const bx::Vec3 at = { 0.0f, 0.0f,   0.0f };
		const bx::Vec3 eye = { 0.0f, 0.0f, -35.0f };

		float time = (float)((SDL_GetPerformanceCounter() - m_timeOffset) / double(SDL_GetPerformanceFrequency()));

		// Set view and projection matrix for view 0.
		{
			float view[16];
			bx::mtxLookAt(view, eye, at);

			float proj[16];
			bx::mtxProj(proj, 60.0f, float(width) / float(height), 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
			bgfx::setViewTransform(s_cube_view, view, proj);

			bgfx::setViewRect(s_cube_view, 0, 0, uint16_t(width), uint16_t(height));
		}


		bgfx::IndexBufferHandle ibh = m_ibh[m_pt];
		uint64_t state = 0
			| (m_r ? BGFX_STATE_WRITE_R : 0)
			| (m_g ? BGFX_STATE_WRITE_G : 0)
			| (m_b ? BGFX_STATE_WRITE_B : 0)
			| (m_a ? BGFX_STATE_WRITE_A : 0)
			| BGFX_STATE_WRITE_Z
			| BGFX_STATE_DEPTH_TEST_LESS
			| BGFX_STATE_CULL_CW
			| BGFX_STATE_MSAA
			| s_ptState[m_pt]
			;

		// Submit 11x11 cubes.
		for (uint32_t yy = 0; yy < 11; ++yy)
		{
			for (uint32_t xx = 0; xx < 11; ++xx)
			{
				float mtx[16];
				bx::mtxRotateXY(mtx, time + xx * 0.21f, time + yy * 0.37f);
				mtx[12] = -15.0f + float(xx) * 3.0f;
				mtx[13] = -15.0f + float(yy) * 3.0f;
				mtx[14] = 0.0f;

				// Set model matrix for rendering.
				bgfx::setTransform(mtx);

				// Set vertex and index buffer.
				bgfx::setVertexBuffer(0, m_vbh);
				bgfx::setIndexBuffer(ibh);

				// Set render states.
				bgfx::setState(state);

				bgfx::submit(s_cube_view, m_program);
			}
		}

		ImGui::End();
	}

	void create_cube_stuff()
	{
		// Create vertex stream declaration.
		PosColorVertex::init();

		m_timeOffset = SDL_GetPerformanceCounter();

		// Create static vertex buffer.
		m_vbh = bgfx::createVertexBuffer(
			// Static data can be passed with bgfx::makeRef
			bgfx::makeRef(s_cubeVertices, sizeof(s_cubeVertices))
			, PosColorVertex::ms_layout
		);

		// Create static index buffer for triangle list rendering.
		m_ibh[0] = bgfx::createIndexBuffer(
			// Static data can be passed with bgfx::makeRef
			bgfx::makeRef(s_cubeTriList, sizeof(s_cubeTriList))
		);

		// Create static index buffer for triangle strip rendering.
		m_ibh[1] = bgfx::createIndexBuffer(
			// Static data can be passed with bgfx::makeRef
			bgfx::makeRef(s_cubeTriStrip, sizeof(s_cubeTriStrip))
		);

		// Create static index buffer for line list rendering.
		m_ibh[2] = bgfx::createIndexBuffer(
			// Static data can be passed with bgfx::makeRef
			bgfx::makeRef(s_cubeLineList, sizeof(s_cubeLineList))
		);

		// Create static index buffer for line strip rendering.
		m_ibh[3] = bgfx::createIndexBuffer(
			// Static data can be passed with bgfx::makeRef
			bgfx::makeRef(s_cubeLineStrip, sizeof(s_cubeLineStrip))
		);

		// Create static index buffer for point list rendering.
		m_ibh[4] = bgfx::createIndexBuffer(
			// Static data can be passed with bgfx::makeRef
			bgfx::makeRef(s_cubePoints, sizeof(s_cubePoints))
		);

		// Create program from shaders.
		m_program = load_program("vs_cubes", "fs_cubes");
	}

	static bgfx::ShaderHandle load_shader(const char* name)
	{
		const char* shaderpath = "???";

		switch (bgfx::getRendererType())
		{
		case bgfx::RendererType::Noop:
		case bgfx::RendererType::Direct3D9:  shaderpath = "app/shaders/dx9/";   break;
		case bgfx::RendererType::Direct3D11:
		case bgfx::RendererType::Direct3D12: shaderpath = "app/shaders/dx11/";  break;
		case bgfx::RendererType::Agc:
		case bgfx::RendererType::Gnm:        shaderpath = "app/shaders/pssl/";  break;
		case bgfx::RendererType::Metal:      shaderpath = "app/shaders/metal/"; break;
		case bgfx::RendererType::Nvn:        shaderpath = "app/shaders/nvn/";   break;
		case bgfx::RendererType::OpenGL:     shaderpath = "app/shaders/glsl/";  break;
		case bgfx::RendererType::OpenGLES:   shaderpath = "app/shaders/essl/";  break;
		case bgfx::RendererType::Vulkan:     shaderpath = "app/shaders/spirv/"; break;
		case bgfx::RendererType::WebGPU:     shaderpath = "app/shaders/spirv/"; break;

		case bgfx::RendererType::Count:
			ASSERT(false, "You should not be here!");
			break;
		}

		String filepath = String(shaderpath) + String(name) + String(".bin");
		auto filedata = read_entire_file_as_bytes(filepath.c_str());
		// TODO: Add a way to take ownership of the allocated data from Vector, so we wouldn't need to
		// copy the buffer here.
		bgfx::ShaderHandle handle = bgfx::createShader(bgfx::copy(filedata.data(), (u32)filedata.size()));
		bgfx::setName(handle, name);

		return handle;
	}

	bgfx::ProgramHandle load_program(const char* vs_name, const char* fs_name)
	{
		bgfx::ShaderHandle vsh = load_shader(vs_name);
		bgfx::ShaderHandle fsh = BGFX_INVALID_HANDLE;
		if (NULL != fs_name)
		{
			fsh = load_shader(fs_name);
		}

		return bgfx::createProgram(vsh, fsh, true /* destroy shaders when program is destroyed */);
	}

	void* get_native_window_handle() { return m_wm_info.info.win.window; }
	void* get_native_display_type() { return nullptr; }

private:
	enum ViewIds : bgfx::ViewId
	{
		s_clear_view,
		s_cube_view,
		s_imgui_view,
		s_view_count,
	};


	BgfxCallback bgfx_callbacks;

	bgfx::VertexBufferHandle m_vbh;
	bgfx::IndexBufferHandle m_ibh[ARRAY_COUNT(s_ptState)];
	bgfx::ProgramHandle m_program;
	s32 m_pt = 0;
	u64 m_timeOffset = 0;
	bool m_r = true;
	bool m_g = true;
	bool m_b = true;
	bool m_a = false;


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
