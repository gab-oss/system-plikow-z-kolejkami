cmake_minimum_required(VERSION 2.8.9)
project (simplefs)

include_directories(.)
include_directories(/usr/include)

add_executable(fsconsole tests/fsconsole.cpp fs.h)