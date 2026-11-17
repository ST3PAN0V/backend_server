# C++ Game Backend

An HTTP backend for an online game, developed as part of the **"C++ for the backend"** course. The project brings together asynchronous networking, game-session logic, state serialization, and a PostgreSQL leaderboard.

## Features

- JSON API for maps, joining a game, player actions, and game-state updates.
- Player tokens and shared game sessions.
- Asynchronous HTTP handling with Boost.Asio and Boost.Beast, using multiple worker threads.
- Game ticks, object generation, collision handling, and player scores.
- Saving and restoring game state through Boost.Serialization.
- PostgreSQL storage for retired players' scores and play times, with a paginated leaderboard.
- Static files for the browser client and Catch2 tests for selected components.

## Stack

| Component | Technology |
| --- | --- |
| Language | C++20 |
| Networking and utilities | Boost 1.78.0 |
| Database client | libpqxx 7.7.4 / PostgreSQL |
| Tests | Catch2 3.1.0 |
| Build | CMake, Conan 1.x (`cmake_multi` generator) |
| Container build | Docker; GCC 11.3 build stage, Ubuntu 22.04 runtime stage |

Dependency versions are defined in [conanfile.txt](conanfile.txt).

## Repository layout

| Path | Purpose |
| --- | --- |
| [game_server/](game_server/) | HTTP server, request handlers, logging, and database access |
| [game_lib/](game_lib/) | Game model, application logic, and serialization |
| [data/](data/) | Game configuration |
| [static/](static/) | Browser client and static assets |
| [tests/](tests/) | Model, object generation, collision, and serialization tests |
| [Dockerfile](Dockerfile) | Container build and server entry point |

## Build locally

The following commands assume a Linux/GCC toolchain with C++20 support, CMake 3.11 or newer, and Conan 1.x. The Dockerfile pins Conan 1.65. The build uses the legacy [Conan `cmake_multi` generator](https://docs.conan.io/en/1.65/reference/generators/cmakemulti.html).

```bash
git clone https://github.com/ST3PAN0V/backend_server.git
cd backend_server
mkdir -p build
cd build
conan install .. --build=missing -s compiler.libcxx=libstdc++11 -s build_type=Debug
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
cd ..
```

Use a **Debug** dependency installation and build: the test configuration refers to `CONAN_BUILD_DIRS_CATCH2_DEBUG`.

## Run

Prepare a PostgreSQL database and export its connection string as `GAME_DB_URL` in your local environment. The database user needs permission to create the leaderboard table and index, insert records, and read them. PostgreSQL is a separate runtime dependency.

From the repository root:

```bash
: "${GAME_DB_URL:?Export GAME_DB_URL before starting the server}"
mkdir -p runtime
./build/game_server/game_server \
  --config-file data/config.json \
  --www-root static/ \
  --tick-period 50 \
  --state-file runtime/game-state.txt \
  --save-state-period 5000
```

Open [http://127.0.0.1:8080/](http://127.0.0.1:8080/). The server uses HTTP on port `8080`.

The example sets a 50 ms game tick and saves state every 5,000 ms of game time. Keep the state directory writable. Pass the timing options explicitly: the current argument structure does not initialize omitted integer options.

### Docker

With `GAME_DB_URL` exported and the database reachable from the container:

```bash
docker build -t stepanov_server .
docker run --rm -p 8080:8080 --env GAME_DB_URL stepanov_server \
  --tick-period 50 \
  --state-file /tmp/game-state.txt \
  --save-state-period 5000
```

The image's entry point supplies the game-configuration and static-file paths. This example stores state inside the temporary container; use a writable mounted location if state must survive container removal.

## API overview

| Endpoint | Purpose |
| --- | --- |
| `/api/v1/maps` | List maps |
| `/api/v1/maps/{id}` | Get a map |
| `/api/v1/game/join` | Join a game and receive a player token |
| `/api/v1/game/players` | List players in the current session |
| `/api/v1/game/state` | Read the game state |
| `/api/v1/game/player/action` | Submit a movement action |
| `/api/v1/game/records` | Read the leaderboard |

Player-specific requests use the token returned by `join` in the `Authorization: Bearer ...` header. See [request_handler.cpp](game_server/src/request_handler.cpp) for methods and validation, and [api_handler.cpp](game_server/src/api_handler.cpp) for the JSON payloads.

## Tests

After the Debug build, run CTest from the directory where the tests are registered:

```bash
cd build/tests
ctest --output-on-failure
```

The configured test executables are `game_server_tests`, `collision_detection_tests`, and `state_serialization_tests`.

## Scope

This is a course project for exploring backend development. PostgreSQL stores leaderboard records; player tokens and game-state persistence are separate parts of the application. No production deployment, load benchmark, or latency guarantee is claimed.
