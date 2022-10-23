add_library(core STATIC
    "def.h"
    "scopeguard.h"
    "utils.h"
    "utils.cpp"
)

target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(core
    PUBLIC
        $<IF:$<BOOL:${IS_INTERNAL_BUILD}>,IS_INTERNAL_BUILD=1,IS_INTERNAL_BUILD=0>
        $<IF:$<BOOL:${USE_ASAN}>,USE_ASAN=1,USE_ASAN=0>
        $<IF:$<CONFIG:Debug>,IS_DEBUG_BUILD=1,IS_DEBUG_BUILD=0>
        $<IF:$<CONFIG:Debug>,IS_RELEASE_BUILD=0,IS_RELEASE_BUILD=1>
        PLATFORM_WIN32=1
        _CRT_SECURE_NO_WARNINGS=1
)
