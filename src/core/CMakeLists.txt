add_library(core STATIC
    "containers/common.h"
    "containers/hashmap.h" 
    "containers/pair.h"
    "containers/string.h"
    "containers/vector.h" 

    "def.h"   
    "simd.h"
    "typetraits.h"
    "utils.h"
    "utils.cpp"
    "own_ptr.h"
    "platform.h"
    
    "platform_win32.cpp"
    "platform_sdl2.cpp"
 "platform_sdl2.h" "containers/list.h")
target_include_directories(core PUBLIC
    ${CMAKE_SOURCE_DIR}/src/
    ${CMAKE_SOURCE_DIR}/src/core
)
target_link_libraries(core
    PUBLIC
        spdlog::spdlog
        SDL2::SDL2
    PRIVATE
        Shlwapi.lib
)

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
        IS_INTERNAL_BUILD=$<BOOL:${IS_INTERNAL_BUILD}>
        IS_DEBUG_BUILD=$<CONFIG:Debug>
        IS_RELEASE_BUILD=$<NOT:$<CONFIG:Debug>>
        SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG
        PLATFORM_WIN32=$<PLATFORM_ID:Windows>
        _CRT_SECURE_NO_WARNINGS=1
)
