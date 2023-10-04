include(${CMAKE_SOURCE_DIR}/cmake/AddBgfxShader.cmake)

add_executable(app
    "imgui_bgfx.cpp"
    "imgui_bgfx.h"
    "main.cpp"
)

add_bgfx_shader(app app_vs_cubes_sh ${CMAKE_CURRENT_SOURCE_DIR}/shaders/vs_cubes.sc)
add_bgfx_shader(app app_fs_cubes_sh ${CMAKE_CURRENT_SOURCE_DIR}/shaders/fs_cubes.sc)
add_bgfx_shader_header(app app_common_sh ${CMAKE_CURRENT_SOURCE_DIR}/shaders/common.sh)
add_bgfx_shader_header(app app_varying_def_sc ${CMAKE_CURRENT_SOURCE_DIR}/shaders/varying.def.sc)


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
