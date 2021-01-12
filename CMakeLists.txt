cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_STANDARD 11)

project(Railway LANGUAGES CXX)

add_library(components
    Railway.h
    Railway.cpp
    Station.cpp
    Train.cpp
)

add_executable(main mainRailway.cpp)

target_link_libraries(main
    components
)
