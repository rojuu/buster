cmake_minimum_required(VERSION 3.10)

project(duster C CXX)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

option(IS_INTERNAL_BUILD "Internal build for, enables development only features" ON)
option(USE_ASAN "Build with ASAN" OFF)

add_subdirectory(src)
add_subdirectory(thirdparty)
