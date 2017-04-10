cmake_minimum_required(VERSION 3.5)
project(intercept)

set(CMAKE_C_STANDARD 11)

include_directories(../../include)
include_directories(../../include/x86_64-linux-gnu)

set(SOURCE_FILES interceptor.c interceptor.h program_headers.c program_headers.h)
add_library(interceptor SHARED ${SOURCE_FILES})

set(CMAKE_C_FLAGS "-O2 -g")
add_library(lib SHARED examples/lib.c examples/lib.h)

add_executable(example examples/example.c)
target_link_libraries(example lib interceptor)
