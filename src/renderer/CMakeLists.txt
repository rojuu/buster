add_library(renderer STATIC
	"renderer.h"
	"renderer_d3d11.cpp"

	"${CMAKE_SOURCE_DIR}/thirdparty/imgui/backends/imgui_impl_dx11.h"
	"${CMAKE_SOURCE_DIR}/thirdparty/imgui/backends/imgui_impl_dx11.cpp"
)

target_include_directories(renderer
	PUBLIC
		"${CMAKE_SOURCE_DIR}/src/renderer"
	PRIVATE
		"${CMAKE_SOURCE_DIR}/thirdparty/imgui/backends/"
)

target_link_libraries(renderer
	PUBLIC
		dear_imgui
	PRIVATE
		core
		stb
		User32.lib
		D3D11.lib
		DXGI.lib
		D3DCompiler.lib
)