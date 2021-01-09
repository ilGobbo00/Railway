cmake_minimum_required(VERSION 3.17)
project(Railway)

#add_library(Railway.h)
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
add_executable(Railway main.cpp Trains.cpp Station.cpp Railway.h)

# Cose da fare:

#  - Scrivere i parametri corretti come vuole Ghidoni