set(CNAME "f2b")
set(VERSION 0.07)

project(${CNAME} C)
cmake_minimum_required(VERSION 2.6)

include(CTest)

option(HARDENING "Enable nardening options" OFF)

set(CMAKE_INSTALL_PREFIX "/usr/local")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -std=c99")
add_definitions("-D_XOPEN_SOURCE=600")

if (HARDENING)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wformat -Wformat-security -Werror=format-security" )
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector --param ssp-buffer-size=4" )
  add_definitions("-D_FORTIFY_SOURCE=2")
endif ()

message(STATUS "Compiler   : ${CMAKE_C_COMPILER} (${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION})")
message(STATUS "- CFLAGS   : ${CMAKE_C_FLAGS}")
message(STATUS "Prefix     : ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "Options:")
message(STATUS "- HARDENING: ${HARDENING}")
message(STATUS "Components:")

add_subdirectory(src)
add_subdirectory(t)
set_property(DIRECTORY "t" PROPERTY COMPILE_FLAGS "-g;-ggdb;-Wall;-Wextra;-pedantic;-O0")

install(DIRECTORY "docs" DESTINATION "${CMAKE_INSTALL_PATH}/share/doc/f2b/")
