add_library(stb INTERFACE
	stb_image.h
	stb_rect_pack.h
	stb_truetype.h
)
target_include_directories(stb INTERFACE .)
