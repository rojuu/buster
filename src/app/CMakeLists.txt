include(${CMAKE_SOURCE_DIR}/cmake/AddBgfxShader.cmake)

add_executable(app
    "imgui_bgfx.cpp"
    "imgui_bgfx.h"
    "main.cpp"
    "bgfx_callback.h"
    "bgfx_callback.cpp")
set_target_properties(app PROPERTIES WIN32_EXECUTABLE $<NOT:$<BOOL:${IS_INTERNAL_BUILD}>>)

add_custom_target(app_shaders)
add_custom_target(app_headers
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/common.sh
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/varying.def.sc
)
add_bgfx_shader(app app_headers app_vs_cubes_sh ${CMAKE_CURRENT_SOURCE_DIR}/shaders/vs_cubes.sc)
add_bgfx_shader(app app_headers app_fs_cubes_sh ${CMAKE_CURRENT_SOURCE_DIR}/shaders/fs_cubes.sc)

target_link_libraries(app PRIVATE
    core
    bx
    bimg
    bgfx
    dear_imgui
    SDL2::SDL2
    SDL2::SDL2main
)

target_include_directories(app
    PRIVATE $<TARGET_PROPERTY:stb,INTERFACE_INCLUDE_DIRECTORIES>
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui
)
