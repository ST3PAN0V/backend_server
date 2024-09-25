cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

find_package(Threads REQUIRED)

#Static library
add_library(GameLib STATIC
    src/tagged.h
    src/geom.h
    src/model.h
    src/model.cpp
    src/loot_data.h
    src/loot_data.cpp
    src/loot_generator.h
    src/loot_generator.cpp
    src/ticker.h
    src/api_handler.h
    src/api_handler.cpp
    src/app.h
    src/app.cpp
    src/collision_detector.h
    src/collision_detector.cpp
)
target_link_libraries(GameLib PUBLIC Threads::Threads CONAN_PKG::boost)

#Server
add_executable(game_server
    src/main.cpp
    src/parser.h
    src/sdk.h
    src/http_server.h
    src/http_server.cpp
    src/boost_json.cpp
    src/json_loader.h
    src/json_loader.cpp
    src/request_handler.h
    src/request_handler.cpp
    src/logger.h
    src/logger.cpp
    src/serialization/manager.h
    src/serialization/representations.h
    src/state_storage.h

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

add_executable(state_serialization_tests
    tests/state-serialization-tests.cpp
)

target_link_libraries(state_serialization_tests PRIVATE GameLib CONAN_PKG::catch2)

include(CTest)
include(${CONAN_BUILD_DIRS_CATCH2_DEBUG}/Catch.cmake)
catch_discover_tests(game_server_tests)
catch_discover_tests(collision_detection_tests)
catch_discover_tests(state_serialization_tests)