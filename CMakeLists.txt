cmake_minimum_required(VERSION 2.8.9)
project (simplefs)

include_directories(.)
include_directories(/usr/include)


add_executable(test_queue tests/test_queues.c fs.h tests/writer.c tests/reader.c)
target_link_libraries(test_queue rt)
