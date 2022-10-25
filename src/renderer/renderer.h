#pragma once

#include "core/IPlatform.h"

#include <vector>
#include <string>

class Renderer {
public:
	explicit Renderer(IPlatform& platform);

	bool init();
	void deinit();

	enum DebugFlags {
		Debug_InfinitelyFastHardware = 0x1,
		Debug_Profiler = 0x2,
		Debug_Stats = 0x4,
		Debug_Text = 0x8,
		Debug_Wireframe = 0x10,
	};

	void set_debug(u32 flags);

	void dbg_print(u16 x, u16 y, const std::string &str);

	void begin_frame();
	void end_frame();

private:
	IPlatform* m_platform;

	s32 m_old_window_width{ 0 };
	s32 m_old_window_height{ 0 };
};