set(CNAME "f2b")
set(VERSION 0.07)

project(${CNAME} C)
cmake_minimum_required(VERSION 2.6)

set(CMAKE_INSTALL_PREFIX "/usr/local")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -std=c99")
add_definitions("-D_XOPEN_SOURCE=600")

include(CTest)

add_subdirectory(src)
add_subdirectory(t)

set_property(DIRECTORY "t" PROPERTY COMPILE_FLAGS "-g;-ggdb;-Wall;-Wextra;-pedantic;-O0")
