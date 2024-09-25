FROM gcc:11.3 as build

# Установка зависимостей
RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.65

# Копируем conanfile.txt в контейнер и устанавливаем зависимости через Conan
COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
    conan install .. --build=missing -s compiler.libcxx=libstdc++11 -s build_type=Debug

# Копируем исходные файлы в контейнер
COPY ./game_lib /app/game_lib
COPY ./game_server /app/game_server
COPY ./tests /app/tests
COPY CMakeLists.txt /app/

# Сборка проекта
RUN cd /app/build && \
    cmake -DCMAKE_BUILD_TYPE=Debug .. && \
    cmake --build .

# Копируем необходимые данные для runtime
COPY ./data /app/data
COPY ./static /app/static

# Сборка образа для выполнения
FROM ubuntu:22.04 as run

# Создаем группу и пользователя для запуска сервера
RUN groupadd -r www && useradd -r -g www www
USER www

# Копируем исполняемые файлы и данные из предыдущего образа
COPY --from=build /app/build/game_server/game_server /app/
COPY ./data /app/data
COPY ./static /app/static

# Определяем точку входа и параметры
ENTRYPOINT ["/app/game_server", "-c", "app/data/config.json", "-w", "app/static/"]
