add_executable(game_server
    src/sdk.h
    src/parser.h
    src/ticker.h
    src/main.cpp
    src/api_handler.h
    src/api_handler.cpp
    src/http_server.h
    src/http_server.cpp
    src/boost_json.cpp
    src/json_loader.h
    src/json_loader.cpp
    src/request_handler.h
    src/request_handler.cpp
    src/logger.h
    src/logger.cpp
    src/state_storage.h
    src/db/database.h
    src/db/database.cpp
    src/db/connection_pool.h
    src/db/tagged_uuid.h
    src/db/tagged_uuid.cpp
)

target_link_libraries(game_server PRIVATE GameLib CONAN_PKG::boost CONAN_PKG::libpqxx)
