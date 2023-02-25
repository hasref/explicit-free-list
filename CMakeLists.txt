cmake_minimum_required(VERSION 3.20)
project(explicit-free-list LANGUAGES CXX)

find_package(fmt REQUIRED)
find_package(Catch2 REQUIRED)
# what should happen is that I make this as a library and then
# have an executable that just tests it.

add_library(alloc SHARED src/memlib.cpp src/mm.cpp)
target_include_directories(alloc PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
target_compile_options(alloc PRIVATE -Wall -Werror -Wpedantic -fsanitize=address)
target_compile_features(alloc PUBLIC cxx_std_20)
target_link_options(alloc PRIVATE -fsanitize=address)
target_link_libraries(alloc PRIVATE fmt::fmt)

# tests
add_executable(tests tests/tests.cpp)
target_include_directories(tests PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
target_compile_options(tests PRIVATE -Wall -Werror -Wpedantic -fsanitize=address)
target_compile_features(tests PRIVATE cxx_std_20)
target_link_options(tests PRIVATE -fsanitize=address)
target_link_libraries(tests PRIVATE Catch2::Catch2 alloc)

# lsp
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_custom_target(
    copy-compile-commands ALL
    ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${CMAKE_CURRENT_LIST_DIR}
    )

