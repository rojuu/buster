add_library(core STATIC
    "def.h"
    "IPlatform.h"
    "scopeguard.h"
    "simd.h"
    "utils.h"
    "utils.cpp"
)

target_include_directories(core PUBLIC ${CMAKE_SOURCE_DIR}/src/)

target_link_libraries(core PUBLIC
    spdlog::spdlog)

# define SPDLOG_ACTIVE_LEVEL to one of those (before including spdlog.h):
# SPDLOG_LEVEL_TRACE,
# SPDLOG_LEVEL_DEBUG,
# SPDLOG_LEVEL_INFO,
# SPDLOG_LEVEL_WARN,
# SPDLOG_LEVEL_ERROR,
# SPDLOG_LEVEL_CRITICAL,
# SPDLOG_LEVEL_OFF

target_compile_definitions(core
    PUBLIC
        IS_INTERNAL_BUILD=$<IF:$<BOOL:${IS_INTERNAL_BUILD}>,1,0>
        USE_ASAN=$<IF:$<BOOL:${USE_ASAN}>,1,0>
        IS_DEBUG_BUILD=$<IF:$<CONFIG:Debug>,1,0>
        IS_RELEASE_BUILD=$<IF:$<CONFIG:Debug>,0,1>
        SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
        PLATFORM_WIN32=1
        _CRT_SECURE_NO_WARNINGS=1
)
