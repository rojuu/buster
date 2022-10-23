#pragma once

#define CONCAT_(x, y) x ## y
#define CONCAT(x, y) CONCAT_(x, y)

#define MACRO_VAR(var) CONCAT(var, __LINE__)

#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))

// Should be used when an argument or variable is not used on purpose, to suppress compiler warnings
#define UNUSED(arg) (void)arg

// Semantically should be used when the developer does not check some
// potential error case at this time, but wants to make it easily searchable. Later on we can search for "UNCHECKED"
// to find all the places in the codebase where some return value was not checked properly
#define UNCHECKED(arg) (void)arg

#include <stdint.h>
#include <stddef.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
static_assert(sizeof(u8) == 1, "size mismatch");
static_assert(sizeof(u16) == 2, "size mismatch");
static_assert(sizeof(u32) == 4, "size mismatch");
static_assert(sizeof(u64) == 8, "size mismatch");

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
static_assert(sizeof(s8) == 1, "size mismatch");
static_assert(sizeof(s16) == 2, "size mismatch");
static_assert(sizeof(s32) == 4, "size mismatch");
static_assert(sizeof(u64) == 8, "size mismatch");

typedef float f32;
typedef double f64;
static_assert(sizeof(f32) == 4, "size mismatch");
static_assert(sizeof(f64) == 8, "size mismatch");

typedef uintptr_t uptr;
typedef intptr_t sptr;
typedef uptr usz;

#define MAX_ALIGN (sizeof(uptr))

static_assert(sizeof(uptr) == sizeof(sptr), "size mismatch");
static_assert(sizeof(uptr) == sizeof(void*), "size mismatch");
static_assert(sizeof(usz) == sizeof(uptr), "size mismatch");

// simd stuffs
#include <emmintrin.h>
#include <immintrin.h>
#include <xmmintrin.h>
#include <pmmintrin.h>

static constexpr auto KiB = 1024ull; // Kibibyte
static constexpr auto MiB = 1024ull * KiB; // Mebibyte
static constexpr auto GiB = 1024ull * MiB; // Gibibyte
static constexpr auto TiB = 1024ull * GiB; // Tebibyte
static constexpr auto PiB = 1024ull * TiB; // Pebibyte

static constexpr auto PAGE_SIZE = 4 * KiB;
