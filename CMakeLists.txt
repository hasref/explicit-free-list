cmake_minimum_required(VERSION 3.20)
project(explicit-free-list LANGUAGES C CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(alloc src/main.cpp)
target_include_directories(alloc PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
target_compile_options(alloc PRIVATE -Wall -Werror -Wpedantic -fsanitize=address -fsanitize=undefined -std=c++20)
target_link_options(alloc PRIVATE -fsanitize=address -fsanitize=undefined)
target_link_libraries(alloc PRIVATE CONAN_PKG::fmt)

add_custom_target(
    copy-compile-commands ALL
    ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${CMAKE_CURRENT_LIST_DIR}
    )
