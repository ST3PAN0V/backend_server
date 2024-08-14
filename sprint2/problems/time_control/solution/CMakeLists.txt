cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
 
add_executable(game_server
	src/main.cpp
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/model.h
	src/model.cpp
	src/tagged.h
	src/boost_json.cpp
	src/json_loader.h
	src/json_loader.cpp
	src/request_handler.cpp
	src/request_handler.h
	src/player_tokens.cpp
	src/player_tokens.h
	src/api_handler.cpp
	src/api_handler.h
	src/players.cpp
	src/players.h
	src/player.cpp
	src/player.h
	src/dog.cpp
	src/dog.h
	src/gamesession.cpp
	src/gamesession.h
	src/common_request_handler.cpp
	src/common_request_handler.h
	src/game_to_json.cpp
	src/game_to_json.h
)
target_link_libraries(game_server PRIVATE Threads::Threads CONAN_PKG::boost)

add_compile_definitions(BOOST_BEAST_USE_STD_STRING_VIEW) 

# используем "импортированную" цель CONAN_PKG::boost
target_include_directories(game_server PRIVATE CONAN_PKG::boost)

