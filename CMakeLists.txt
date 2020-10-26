cmake_minimum_required(VERSION 3.17)
project(UltraMultiThread)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_PREFIX_PATH .)

set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

find_package(umt)
include_directories(${umt_INCLUDE_DIR})

add_compile_options(-O3)

add_executable(example_subscription example/subscription.cpp)
target_link_libraries(example_subscription ${umt_LIBS})

find_package(Boost REQUIRED COMPONENTS python3)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

add_library(py_example SHARED example/py_example.cpp)
target_link_libraries(py_example ${umt_LIBS} Boost::python3 Python3::Python)
