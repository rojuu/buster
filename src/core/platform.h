#pragma once

#include "def.h"
#include "containers/common.h"


namespace bstr::core::platform {

#if PLATFORM_WIN32
static constexpr auto PATH_DELIMITER = '\\';
#else
#error "Unsupported platform"
#endif      

struct WindowHandle
{
    void* nwh{}; // native window handle
    void* ndt{}; // native display type (mostly useful for X11)
};

struct WindowSize
{
    s32 width{};
    s32 height{};
};

void print(const char* msg);

typedef void OpaqueFunction();
typedef OpaqueFunction* OpaqueFunctionPtr;

void* load_library(const char* library_filename);
void free_library(void* library);
OpaqueFunctionPtr get_proc_address(void* library, const char* proc_name);

bool copy_file(const char* src, const char* dst);
bool file_exists(const char* filename);
void set_current_working_directory(const char* cwd);

f64 get_highresolution_time_seconds();
WindowSize get_window_size();
WindowHandle get_native_window_handle();
string get_current_working_directory();
u64 get_file_modify_time(const char* filename);

}