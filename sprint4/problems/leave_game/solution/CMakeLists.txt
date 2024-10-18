cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

find_package(Threads REQUIRED)

add_subdirectory(game_lib)
add_subdirectory(game_server)
add_subdirectory(tests)