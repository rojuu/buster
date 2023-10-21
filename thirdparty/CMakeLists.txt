add_subdirectory(SDL EXCLUDE_FROM_ALL)
add_subdirectory(stb EXCLUDE_FROM_ALL)
add_subdirectory(zlib EXCLUDE_FROM_ALL)

target_include_directories(zlib
	PUBLIC
		"${CMAKE_CURRENT_SOURCE_DIR}/zlib"
		"${CMAKE_BINARY_DIR}/thirdparty/zlib"
)

add_subdirectory(spdlog EXCLUDE_FROM_ALL)

add_library(dear_imgui STATIC
	"imgui/imgui.cpp"
	"imgui/imgui_demo.cpp"
	"imgui/imgui_draw.cpp"
	"imgui/imgui_tables.cpp"
	"imgui/imgui_widgets.cpp"
)

target_include_directories(dear_imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
