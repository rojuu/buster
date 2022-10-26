#pragma once

#include "def.h"
#include "typetraits.h"

#include <limits>
#include <spdlog/spdlog.h>


#if defined(_MSC_VER)
#if _MSC_VER < 1300
#define DEBUG_TRAP() __asm int 3; /* Trap to debugger! */
#else
#define DEBUG_TRAP() __debugbreak()
#endif
#else
#define DEBUG_TRAP() __builtin_trap()
#endif

#define LOG_TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)

void logger_init();

// Stop in debugger if condition is false, and log a message
#define ASSERT(cond, msg)  do { if (!(cond)) { LOG_DEBUG("{}", msg); DEBUG_TRAP(); } } while(0)

// Add this to any codebranch that shuold never be reached. Atm it causes a debug trap, but maybe should also call abort or throw exception?
#define ASSERT_UNREACHED() do { LOG_DEBUG("{}", "Unreached code path!"); DEBUG_TRAP(); } while(0)

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
using numeric_limits = std::numeric_limits<T>;


template<class T>
inline void zero_struct(T* s) {
    memset(s, 0, sizeof(*s));
}

template<class T>
constexpr remove_reference_t<T>&& move(T&& t) noexcept
{
    return static_cast<typename remove_reference<T>::type&&>(t);
}

template<class T>
constexpr T&& forward(remove_reference_t<T>& t) noexcept
{
    return static_cast<T&&>(t);
}

template<class T>
constexpr T&& forward(remove_reference_t<T>&& t) noexcept
{
    return static_cast<T&&>(t);
}


template<class F>
class ScopeGuard {
    F func;
public:
    ScopeGuard(F&& f)
        : func(forward<F&&>(f))
    {
    }
    ~ScopeGuard()
    {
        func();
    }
};

template<class F>
ScopeGuard(F&&)->ScopeGuard<F>;


