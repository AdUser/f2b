set(CNAME "f2b")
set(VERSION 0.07)

project(${CNAME} C)
cmake_minimum_required(VERSION 2.6)

set(CMAKE_INSTALL_PREFIX "/usr/local")

include(CTest)

add_subdirectory(src)
add_subdirectory(t)

set_property(DIRECTORY "t" PROPERTY COMPILE_FLAGS "-g;-ggdb;-Wall;-Wextra;-pedantic;-O0")
