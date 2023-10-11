add_library(core STATIC
    "containers/common.h"
    "containers/hashmap.h" 
    "containers/list.h"
    "containers/pair.h"
    "containers/span.h"
    "containers/string.h"
    "containers/string_view.h"
    "containers/vector.h" 

    "def.h"   
    "non_null.h"
    "out_ptr.h"
    "simd.h"
    "smart_ptr.h"
    "string_builder.h"
    "utils.h"
    "utils.cpp"

    "platform.h"
    "platform_sdl2.h"
    "platform_sdl2.cpp"
    "platform_win32.cpp"
)
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
