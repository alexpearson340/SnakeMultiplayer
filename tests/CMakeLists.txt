add_executable(
    unit_tests
    server_config_test.cpp
    replay_determinism_test.cpp
)

target_link_libraries(
    unit_tests
    GTest::gtest_main
    snake_server_lib
)

# The replay determinism test shells out to the real server binary
target_compile_definitions(
    unit_tests PRIVATE
    SNAKE_SERVER_BIN="$<TARGET_FILE:snake_server>"
    FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

add_dependencies(unit_tests snake_server)

include(GoogleTest)
gtest_discover_tests(unit_tests)
