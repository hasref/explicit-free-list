#! /usr/bin/bash

cd build
conan install ../conanfile.txt --build=missing --profile default -s build_type=Debug -of ./
cmake ../ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake && make -j 8
