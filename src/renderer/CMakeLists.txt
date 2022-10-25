add_library(renderer STATIC
	"imgui_bgfx.h"
	"imgui_bgfx.cpp"
	"renderer.h"
	"renderer_bgfx.cpp"
 )

target_include_directories(renderer PUBLIC ${CMAKE_SOURCE_DIR}/src/)

target_link_libraries(renderer PRIVATE
PUBLIC
	dear_imgui
PRIVATE
	core
    bx
    bimg
    bgfx
)
