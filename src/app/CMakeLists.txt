add_executable(app
    "main.cpp")

target_link_libraries(app PRIVATE
    core
    SDL2::SDL2
    SDL2::SDL2main
    bx
    bimg
    bgfx
)

target_include_directories(app PRIVATE $<TARGET_PROPERTY:stb,INTERFACE_INCLUDE_DIRECTORIES>)

target_compile_options(app
    PRIVATE
        $<$<BOOL:${USE_ASAN}>:/fsanitize=address>
)