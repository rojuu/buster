include(${CMAKE_SOURCE_DIR}/cmake/AddBgfxShader.cmake)

add_executable(app
    "imgui_bgfx.cpp"
    "imgui_bgfx.h"
    "main.cpp"
)

add_bgfx_shader(${CMAKE_CURRENT_SOURCE_DIR}/shaders/vs_cubes.sc app)
add_bgfx_shader(${CMAKE_CURRENT_SOURCE_DIR}/shaders/fs_cubes.sc app)

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
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui)

target_compile_options(app
    PRIVATE
        $<$<BOOL:${USE_ASAN}>:/fsanitize=address>
)