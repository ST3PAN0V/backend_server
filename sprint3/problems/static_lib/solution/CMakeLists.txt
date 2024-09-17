cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

find_package(Threads REQUIRED)

#Static library
add_library(GameLib STATIC
    src/model.cpp
    src/tagged.h
    src/loot_data.cpp
    src/loot_data.h
    src/loot_generator.cpp
    src/loot_generator.h
    src/api_handler.cpp
    src/api_handler.h
    src/app.cpp
    src/app.h
    src/ticker.h
)
target_link_libraries(GameLib PUBLIC Threads::Threads CONAN_PKG::boost)

#Server
add_executable(game_server
    src/main.cpp
    src/http_server.cpp
    src/http_server.h
    src/sdk.h
    src/model.h
    src/boost_json.cpp
    src/json_loader.h
    src/json_loader.cpp
    src/request_handler.cpp
    src/request_handler.h
    src/logger.h
    src/logger.cpp
    src/parser.h
)
target_link_libraries(game_server PRIVATE GameLib CONAN_PKG::boost)

#Tests
add_executable(game_server_tests
    tests/model_tests.cpp
    tests/loot_generator_tests.cpp
)

# Настройки для тестов
target_link_libraries(game_server_tests PRIVATE GameLib CONAN_PKG::catch2)
