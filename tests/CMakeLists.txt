cmake_minimum_required(VERSION 2.8.9)
project (fsconsole)
set (CMAKE_CXX_STANDARD 11)

include_directories(../include)

add_executable(fsconsole fsconsole.cpp ../src/fs.c)