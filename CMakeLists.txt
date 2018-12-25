cmake_minimum_required(VERSION 2.8.9)
project (simplefs)

include_directories(/usr/include)
include_directories(include)

add_executable(simplefs src/main.c src/fs.c src/queue.c)