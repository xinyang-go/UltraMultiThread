cmake_minimum_required(VERSION 3.5)
project(UltraMultiThread)

set(CMAKE_CXX_STANDARD 17)

list(APPEND CMAKE_PREFIX_PATH .)

find_package(umt)
include_directories(${umt_INCLUDE_DIR})

option(WITH_EXAMPLE OFF)
if (WITH_EXAMPLE)
    add_subdirectory(example)
endif ()