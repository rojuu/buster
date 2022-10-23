#pragma once

#include "def.h"
#include <stdio.h>
#include <stdlib.h>

#include <algorithm>


#if 0
enum LogVerbosity
{
    Log_ASSERT,
    Log_FATAL,
    Log_ERROR,
    Log_WARNING,
    Log_INFO,
    Log_DEBUG,
    Log_VERBOSE,
};

#define LOG(verbosity, msg)       do { (void)(Log_ ## verbosity); printf("[ "#verbosity " ] %s\n", msg);             } while(0)
#define LOGF(verbosity, fmt, ...) do { (void)(Log_ ## verbosity); printf("[ "#verbosity " ] " fmt "\n", __VA_ARGS__);} while(0)
#endif
#define LOG(...) (void)0

#if defined(_MSC_VER)
#if _MSC_VER < 1300
#define DEBUG_TRAP() __asm int 3; /* Trap to debugger! */
#else
#define DEBUG_TRAP() __debugbreak()
#endif
#else
#define DEBUG_TRAP() __builtin_trap()
#endif

// Stop in debugger if condition is false, and log a message
#define ASSERT(cond, msg)  do { if (!(cond)) { LOG(ASSERT, msg); DEBUG_TRAP(); } } while(0)

// Add this to any codebranch that shuold never be reached. Atm it causes a debug trap, but maybe should also call abort or throw exception?
#define ASSERT_UNREACHED() do { LOG(ASSERT, "Unreached code path!"); DEBUG_TRAP(); } while(0)

// Literally does the same thing as assert, but semantically should be used when the developer does not check some
// potential error case at this time, but wants to make it easily searchable. Later on we can search for "UNCHECKED"
// to find all the places in the codebase where some return value was not checked properly
// Similar UNCECKED macro also exists in def.h that does not assert
#define ASSERT_UNCHECKED(cond, msg) ASSERT(cond, msg)

inline bool is_power_of_two(uptr value)
{
    bool result = (value & (value - 1)) == 0;
    return result;
}

inline uptr align_forwards(uptr value, usz alignment)
{
    ASSERT(is_power_of_two(alignment), "alignment not power of two");
    uptr result = value;
    usz modulo = value & (alignment - 1);
    if (modulo != 0)
    {
        result += alignment - modulo;
    }
    ASSERT(result % alignment == 0, "result not aligned");
    return result;
}

inline uptr align_backwards(uptr value, usz alignment)
{
    ASSERT(is_power_of_two(alignment), "alignment not power of two");
    uptr result = value & alignment;
    return result;
}

template<class T>
inline void zero_struct(T* s) {
    memset(s, 0, sizeof(*s));
}

