#pragma once

#include "def.h"
#include "containers/common.h"

struct PlatformWindowHandle
{
    void* nwh; // native window handle
    void* ndt; // native display type (mostly useful for X11)
};

struct WindowSize
{
    s32 width;
    s32 height;
};

void platform_print(const char* msg);

typedef void PlatformOpaqueFunction();
typedef PlatformOpaqueFunction* PlatformOpaqueFunctionPtr;

void* platform_load_library(const char* library_filename);
void platform_free_library(void* library);
PlatformOpaqueFunctionPtr platform_get_proc_address(void* library, const char* proc_name);

bool platform_copy_file(const char* src, const char* dst);
bool platform_file_exists(const char* filename);
void platform_set_current_working_directory(const char* cwd);

f64 platform_get_highresolution_time_seconds();
WindowSize platform_get_window_size();
PlatformWindowHandle platform_get_native_window_handle();
String platform_get_current_working_directory();
u64 platform_get_file_modify_time(const char* filename);
