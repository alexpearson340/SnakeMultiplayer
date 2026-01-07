cmake_minimum_required(VERSION 3.14)
project(zsnake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COMPILE_COMMANDS_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

find_package(Curses REQUIRED)

include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(json)

# ENGINE LIB
add_library(
    engine_lib STATIC
    src/engine/Engine.cpp
)

target_include_directories(
    engine_lib PUBLIC
    include
    ${CURSES_INCLUDE_DIR}
)

target_link_libraries(
    engine_lib
    nlohmann_json::nlohmann_json
    ${CURSES_LIBRARIES}
)

# CLIENT LIB
add_library(
    snake_client_lib STATIC
    src/snake_client/SnakeClient.cpp
    src/snake_client/NetworkClient.cpp
)

target_include_directories(
    snake_client_lib PUBLIC
    include
)

target_link_libraries(
    snake_client_lib
    engine_lib
)

# SERVER LIB
add_library(
    snake_server_lib STATIC
    src/snake_server/SnakeServer.cpp
    src/snake_server/NetworkServer.cpp
)

target_include_directories(
    snake_server_lib PUBLIC
    include
)

target_link_libraries(
    snake_server_lib
    engine_lib
)

# CLIENT EXECUTABLE
add_executable(
    snake_client
    src/snake_client/main.cpp
)

target_link_libraries(
    snake_client
    snake_client_lib
)

# SERVER EXECUTABLE
add_executable(
    snake_server
    src/snake_server/main.cpp
)

target_link_libraries(
    snake_server
    snake_server_lib
)
# copy compile_commands.json to project root
