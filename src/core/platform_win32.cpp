// Copyright (c) 2021-2023, Roni Juppi <roni.juppi@gmail.com>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#pragma warning(push, 0)
#include <windows.h>
#include <shlwapi.h>
#pragma warning(pop)

#include <intrin.h>

#include "def.h"
#include "utils.h"
#include "platform.h"

#include <stdio.h>

void win32_print_last_error(const char* msg)
{
    char err[256] = { 0 };
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err, ARRAY_COUNT(err) - 1, NULL);
    LOG_ERROR("{}: {}", msg, err);
}

void* platform_load_library(const char* library_filename)
{
    HMODULE library_module = LoadLibraryA(library_filename);
    return library_module;
}

void platform_free_library(void* library)
{
    BOOL result = FreeLibrary((HMODULE)library);
    if (!result)
    {
        win32_print_last_error("FreeLibrary");
    }
}

bool platform_copy_file(const char* src, const char* dst)
{
    BOOL result = CopyFileA(src, dst, false);
    if (!result)
    {
        win32_print_last_error("CopyFileA");
    }
    return result;
}

bool platform_file_exists(const char* filename)
{
    BOOL result = PathFileExistsA(filename);
    return result;
}

PlatformOpaqueFunctionPtr platform_get_proc_address(void* library, const char* proc_name)
{
    PlatformOpaqueFunctionPtr func = (PlatformOpaqueFunctionPtr)GetProcAddress((HMODULE)library, proc_name);
    if (!func) {
        win32_print_last_error("GetProcAddress");
    }
    return func;
}

void platform_print(const char* str)
{
    fprintf(stdout, "%s", str);
    OutputDebugStringA(str);
}

void platform_set_current_working_directory(const char* cwd)
{
    bool res = SetCurrentDirectoryA(cwd);
    ASSERT(res, "");
}

String platform_get_current_working_directory()
{
    String buffer;
    auto buffer_len = GetCurrentDirectoryA(0, nullptr);
    buffer.resize((usz)buffer_len);
    GetCurrentDirectoryA((DWORD)buffer.size(), buffer.data());
    return buffer;
}

u64 platform_get_file_modify_time(const char* filename)
{
    u64 result = 0;
    HANDLE file_handle = CreateFileA(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_handle != INVALID_HANDLE_VALUE)
    {
        FILETIME creation_time;
        FILETIME last_access_time;
        FILETIME last_write_time;

        BOOL success = GetFileTime(file_handle, &creation_time, &last_access_time, &last_write_time);
        if (success)
        {
            ULARGE_INTEGER large_integer = { 0 };
            large_integer.LowPart = last_write_time.dwLowDateTime;
            large_integer.HighPart = last_write_time.dwHighDateTime;
            result = large_integer.QuadPart;
        }

        CloseHandle(file_handle);
    }

    return result;
}
