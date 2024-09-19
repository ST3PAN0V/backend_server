cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

find_package(Threads REQUIRED)

#Static library
add_library(GameLib STATIC
    src/model.cpp
    src/loot_data.cpp
    src/loot_generator.cpp
    src/api_handler.cpp
    src/app.cpp
    src/collision_detector.cpp
)
target_link_libraries(GameLib PUBLIC Threads::Threads CONAN_PKG::boost)

#Server
add_executable(game_server
    src/main.cpp
    src/http_server.cpp
    src/boost_json.cpp
    src/json_loader.cpp
    src/request_handler.cpp
    src/logger.cpp
)
target_link_libraries(game_server PRIVATE GameLib CONAN_PKG::boost)

#Tests
add_executable(game_server_tests
    tests/model_tests.cpp
    tests/loot_generator_tests.cpp
)

target_link_libraries(game_server_tests PRIVATE GameLib CONAN_PKG::catch2)

add_executable(collision_detection_tests
    tests/collision-detector-tests.cpp
)

target_link_libraries(collision_detection_tests PRIVATE GameLib CONAN_PKG::catch2)

include(CTest)
include(${CONAN_BUILD_DIRS_CATCH2_DEBUG}/Catch.cmake)
catch_discover_tests(game_server_tests)
catch_discover_tests(collision_detection_tests)
