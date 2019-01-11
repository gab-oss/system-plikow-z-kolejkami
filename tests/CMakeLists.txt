cmake_minimum_required(VERSION 2.8.9)
project (fsconsole)

include_directories(/usr/include)
include_directories(../include)

set (CMAKE_CXX_STANDARD 11)

add_executable(simplefs  ../src/fs.c ../src/queue.c fsconsole.cpp)
