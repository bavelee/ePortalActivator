cmake_minimum_required(VERSION 3.10)
project(epact C)

SET(CMAKE_EXE_LINKER_FLAGS "-static")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s -Wall -O2")

set(CMAKE_C_STANDARD 99)

add_executable(epact main.c httptool.c httptool.h)
