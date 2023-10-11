add_executable(main
    "main.cpp"

    "${CMAKE_SOURCE_DIR}/thirdparty/imgui/backends/imgui_impl_sdl2.h"
    "${CMAKE_SOURCE_DIR}/thirdparty/imgui/backends/imgui_impl_sdl2.cpp"
)
set_target_properties(main PROPERTIES WIN32_EXECUTABLE $<NOT:$<BOOL:${IS_INTERNAL_BUILD}>>)

target_include_directories(main
    PRIVATE
        "${CMAKE_SOURCE_DIR}/thirdparty/imgui/backends"
)

target_link_libraries(main
    PRIVATE
        core
        renderer
        SDL2::SDL2main
)

#target_include_directories(engine
#    PRIVATE $<TARGET_PROPERTY:stb,INTERFACE_INCLUDE_DIRECTORIES>
#)
