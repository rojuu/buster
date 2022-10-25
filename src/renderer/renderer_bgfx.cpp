#include "renderer.h"

#include <bx/bx.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include "imgui_bgfx.h"

static constexpr bgfx::ViewId s_clear_view = 0;

Renderer::Renderer(IPlatform& platform)
	: m_platform(&platform)
{

}

bool Renderer::init()
{
	//bgfx::renderFrame(); // Call before init to force rendering from same thread
	bgfx::Init init;
	init.platformData.ndt = m_platform->get_native_display_type();
	init.platformData.nwh = m_platform->get_native_window_handle();

#if IS_DEBUG_BUILD
	init.debug = true;
#endif

	auto [window_width, window_height] = m_platform->get_window_size();
	m_old_window_width = window_width;
	m_old_window_height = window_height;
	init.resolution.width = (uint32_t)window_width;
	init.resolution.height = (uint32_t)window_height;
	init.resolution.reset = BGFX_RESET_VSYNC;

	if (!bgfx::init(init)) {
		return false;
	}

	bgfx::setViewClear(s_clear_view, BGFX_CLEAR_COLOR);
	bgfx::setViewRect(s_clear_view, 0, 0, bgfx::BackbufferRatio::Equal);

	imguiCreate();

	return true;
}

void Renderer::deinit()
{
	imguiDestroy();
	bgfx::shutdown();
}

void Renderer::set_debug(u32 flags)
{
	u32 bgfx_flags = BGFX_DEBUG_NONE;
	if (flags & Debug_InfinitelyFastHardware) bgfx_flags |= BGFX_DEBUG_IFH;
	if (flags & Debug_Profiler) bgfx_flags |= BGFX_DEBUG_PROFILER;
	if (flags & Debug_Stats) bgfx_flags |= BGFX_DEBUG_STATS;
	if (flags & Debug_Text) bgfx_flags |= BGFX_DEBUG_TEXT;
	if (flags & Debug_Wireframe) bgfx_flags |= BGFX_DEBUG_WIREFRAME;
	bgfx::setDebug(bgfx_flags);
}

void Renderer::dbg_print(u16 x, u16 y, const std::string& str)
{
	bgfx::dbgTextPrintf(x, y, 0x0f, "%s", str.c_str());
}

void Renderer::begin_frame()
{
	bgfx::dbgTextClear();

	auto [window_width, window_height] = m_platform->get_window_size();
	
	if (window_width != m_old_window_width || window_height != m_old_window_height) {
		bgfx::reset((u32)window_width, (u32)window_height, BGFX_RESET_NONE);
		bgfx::setViewRect(s_clear_view, 0, 0, bgfx::BackbufferRatio::Equal);
	}

	m_old_window_width = window_width;
	m_old_window_height = window_height;

	bgfx::touch(s_clear_view);
	imguiBeginFrame(s_clear_view);
}

void Renderer::end_frame()
{
	imguiEndFrame();
	bgfx::frame();
}
