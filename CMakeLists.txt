cmake_minimum_required(VERSION 3.20)
project(explicit-free-list LANGUAGES CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

# what should happen is that I make this as a library and then
# have an executable that just tests it.

add_library(alloc SHARED src/memlib.cpp src/mm.cpp)
target_include_directories(alloc PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
target_compile_options(alloc PRIVATE -Wall -Werror -Wpedantic -fsanitize=address -fsanitize=undefined)
target_compile_features(alloc PUBLIC cxx_std_20)
target_link_options(alloc PRIVATE -fsanitize=address -fsanitize=undefined)
target_link_libraries(alloc PRIVATE CONAN_PKG::fmt)

# tests
add_executable(tests tests/tests.cpp)
target_include_directories(tests PRIVATE include)
target_compile_options(tests PRIVATE -Wall -Werror -Wpedantic -fsanitize=address -fsanitize=undefined)
target_link_options(tests PRIVATE -fsanitize=address -fsanitize=undefined)
target_link_libraries(tests PRIVATE CONAN_PKG::fmt CONAN_PKG::catch2 alloc)

# lsp
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_custom_target(
    copy-compile-commands ALL
    ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${CMAKE_CURRENT_LIST_DIR}
    )

