cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(Railway LANGUAGES CXX)

add_library(components
    Railway.h
    Railway.cpp
    Station.cpp
    Trains.cpp
)

add_executable(main mainRailway.cpp)

target_link_libraries(main
    components
)
