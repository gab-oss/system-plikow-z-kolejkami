cmake_minimum_required(VERSION 2.8.9)
project (simplefs)

include_directories(.)
include_directories(/usr/include)

set(CMAKE_CXX_STANDARD 11)

add_executable(fsconsole tests/fsconsole.cpp fs.h)
add_executable(test tests/single_proc_test.c fs.h)
