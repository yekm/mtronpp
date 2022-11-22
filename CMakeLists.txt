cmake_minimum_required(VERSION 3.5)

project(mtronpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(mtronpp mtronpp.cpp)

target_link_libraries(mtronpp pthread X11)

