cmake_minimum_required(VERSION 3.11)

add_library(GameLib STATIC
    model/tagged.h
    model/geom.h
    model/model.h
    model/model.cpp
    model/collision_detector.h
    model/collision_detector.cpp
    app/loot_data.h
    app/loot_data.cpp
    app/loot_generator.h
    app/loot_generator.cpp
    app/app.h
    app/app.cpp
    serialization/manager.h
    serialization/representations.h
)

target_include_directories(GameLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
)

target_link_libraries(GameLib PUBLIC Threads::Threads CONAN_PKG::boost)
