cmake_minimum_required(VERSION 3.5)
project(intercept)

set(CMAKE_C_STANDARD 11)

set(SOURCE_FILES interceptor.c interceptor.h)
add_library(intercept SHARED ${SOURCE_FILES})