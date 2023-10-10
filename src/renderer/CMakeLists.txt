add_library(renderer STATIC
	"renderer.h"
	"renderer_d3d11.cpp"
)

target_include_directories(renderer PUBLIC ${CMAKE_SOURCE_DIR}/src/renderer)

target_link_libraries(renderer
	PRIVATE
		core
		stb
		User32.lib
		D3D11.lib
		DXGI.lib
		D3DCompiler.lib
)