#pragma once

#include "def.h"

class IPlatform {
public:
	struct WindowSize {
		s32 width;
		s32 height;
	};
	virtual WindowSize get_window_size() = 0;

	virtual void* get_native_window_handle() = 0;
	virtual void* get_native_display_type() = 0;
};
