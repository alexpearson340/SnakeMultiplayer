# https://github.com/cpp-best-practices/cppbestpractices/blob/master/02-Use_the_Tools_Available.md

option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

add_library(project_warnings INTERFACE)

set(CLANG_WARNINGS
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wconversion
    -Wsign-conversion
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
    -Wimplicit-fallthrough
)

set(GCC_WARNINGS
    ${CLANG_WARNINGS}
    -Wmisleading-indentation
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wuseless-cast
)

if(WARNINGS_AS_ERRORS)
    list(APPEND CLANG_WARNINGS -Werror)
    list(APPEND GCC_WARNINGS -Werror)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(project_warnings INTERFACE ${CLANG_WARNINGS})
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(project_warnings INTERFACE ${GCC_WARNINGS})
else()
    message(WARNING "No warning flags set for compiler: '${CMAKE_CXX_COMPILER_ID}'")
endif()
