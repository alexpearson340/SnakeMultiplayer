cmake_minimum_required(VERSION 3.25)
project(SnakeMultiplayer)
add_compile_options(-g)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COMPILE_COMMANDS_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

include(cmake/CompilerWarnings.cmake)

find_package(Curses REQUIRED)

include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SYSTEM
)
FetchContent_MakeAvailable(json)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.15.3
    SYSTEM
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    SYSTEM
)
FetchContent_MakeAvailable(googletest)

# On macOS/BSD there is no native epoll; epoll-shim provides the epoll API
# (sys/epoll.h) implemented on top of kqueue.Linux uses the kernel's epoll directly.
if(APPLE)
    FetchContent_Declare(
        epoll-shim
        GIT_REPOSITORY https://github.com/jiixyj/epoll-shim.git
        GIT_TAG v0.0.20240608
        SYSTEM
    )
    set(BUILD_SHARED_LIBS OFF)
    FetchContent_MakeAvailable(epoll-shim)
    set(EPOLL_SHIM_LIB epoll-shim)
endif()

# CLIENT LIB
add_library(
    snake_client_lib STATIC
    src/snake_client/SnakeClient.cpp
    src/snake_client/NetworkClient.cpp
)

target_include_directories(
    snake_client_lib PUBLIC
    include
    ${CURSES_INCLUDE_DIR}
)

target_link_libraries(
    snake_client_lib
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    ${CURSES_LIBRARIES}
    project_warnings
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
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    ${EPOLL_SHIM_LIB}
    project_warnings
)

# BOT LIB
add_library(
    snake_bot_lib STATIC
    src/snake_bot/BotNetwork.cpp
    src/snake_bot/SnakeBot.cpp
    src/snake_bot/Pathfinder.cpp
)

target_include_directories(
    snake_bot_lib PUBLIC
    include
)

target_link_libraries(
    snake_bot_lib
    snake_client_lib
    nlohmann_json::nlohmann_json
    ${EPOLL_SHIM_LIB}
    project_warnings
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

# BOT EXECUTABLE
add_executable(
    snake_bot
    src/snake_bot/main.cpp
)

target_link_libraries(
    snake_bot
    snake_bot_lib
)

# TESTS
enable_testing()
add_subdirectory(tests)

# copy compile_commands.json to project root
add_custom_target(
    copy-compile-commands ALL
    ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/compile_commands.json   
        ${CMAKE_SOURCE_DIR}/compile_commands.json
    DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json   
)