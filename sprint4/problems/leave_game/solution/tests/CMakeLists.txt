add_executable(game_server_tests
    model_tests.cpp
    loot_generator_tests.cpp
)

target_link_libraries(game_server_tests PRIVATE GameLib CONAN_PKG::catch2)

add_executable(collision_detection_tests
    collision-detector-tests.cpp
)

target_link_libraries(collision_detection_tests PRIVATE GameLib CONAN_PKG::catch2)

add_executable(state_serialization_tests
    state-serialization-tests.cpp
)

target_link_libraries(state_serialization_tests PRIVATE GameLib CONAN_PKG::catch2)

include(CTest)
include(${CONAN_BUILD_DIRS_CATCH2_DEBUG}/Catch.cmake)
catch_discover_tests(game_server_tests)
catch_discover_tests(collision_detection_tests)
catch_discover_tests(state_serialization_tests)