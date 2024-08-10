cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

include_directories(includes includes/logger includes/network includes/network/rest_api)

add_executable(game_server
	src/logger/logger.cpp
	src/network/http_server.cpp
	src/network/request_handler.cpp
	src/network/rest_api/api.cpp
	src/network/rest_api/file.cpp
	src/network/rest_api/response_base.cpp
	src/application.cpp
	src/gameplay.cpp
	src/model.cpp
	src/json_loader.cpp
	src/ticker.cpp
	src/main.cpp
)

target_link_libraries(game_server PRIVATE CONAN_PKG::boost)
