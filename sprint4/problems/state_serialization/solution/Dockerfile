FROM gcc:11.3 as build

RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.65

# копируем conanfile.txt в контейнер и запускаем conan install
COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
    conan install .. --build=missing -s compiler.libcxx=libstdc++11 -s build_type=Debug

# только после этого копируем остальные иходники
COPY ./src /app/src
COPY ./tests /app/tests
COPY CMakeLists.txt /app/

RUN cd /app/build && \
    cmake -DCMAKE_BUILD_TYPE=Debug .. && \
    cmake --build .

RUN cd /app/build && ls && sleep 5

COPY ./data /app/data
COPY ./static /app/static


FROM ubuntu:22.04 as run

RUN groupadd -r www && useradd -r -g www www
USER www

COPY --from=build /app/build/game_server /app/
COPY --from=build /app/build/collision_detection_tests /app/build/
COPY --from=build /app/build/state_serialization_tests /app/build/
COPY ./data /app/data
COPY ./static /app/static

ENTRYPOINT ["/app/game_server", "-c", "app/data/config.json", "-w", "app/static/"]
