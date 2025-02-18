cmake_minimum_required(VERSION 3.29)
project(raytracer)

set(CMAKE_CXX_STANDARD 17)

add_executable(raytracer main.cpp)
add_executable(pi pi.cpp)
add_executable(integrate_x_sq integrate_x_sq.cpp)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()
