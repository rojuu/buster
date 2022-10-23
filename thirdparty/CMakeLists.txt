add_subdirectory(SDL EXCLUDE_FROM_ALL)
add_subdirectory(stb EXCLUDE_FROM_ALL)

set(BGFX_BUILD_EXAMPLES OFF)
add_subdirectory(bgfx.cmake EXCLUDE_FROM_ALL)

add_subdirectory(spdlog)
