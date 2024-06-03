cmake_minimum_required(VERSION 3.4)

set(CNAME "f2b")
set(VERSION "0.6.1")

project(${CNAME} C)

include(CTest)

find_package(PkgConfig REQUIRED)

option(WITH_CLIENT    "Simple client for configuring daemon" ON)
option(WITH_CSOCKET   "Unix control socket support for daemon" ON)
option(WITH_HARDENING "Enable hardening options" ON)
option(WITH_READLINE  "Use readline library for client" ON)
option(WITH_PCRE      "Build pcre-compatible filter" ON)
option(WITH_REDIS     "Build redis source/backend" OFF)
option(WITH_IPSET     "Build native ipset backend" OFF)

set(INIT_SCRIPT "OFF") # valid variants: "off", "sysvinit", "openrc", "systemd"

if (NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr")
endif ()
include(GNUInstallDirs)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -std=c99")
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  add_definitions("-D_GNU_SOURCE")
else ()
  include_directories(AFTER SYSTEM "/usr/local/include")
  link_directories("/usr/local/lib")
endif ()

if (WITH_HARDENING)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wformat -Wformat-security -Werror=format-security" )
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector --param ssp-buffer-size=4" )
  add_definitions("-D_FORTIFY_SOURCE=2")
endif ()

pkg_check_modules(READLINE "readline" REQUIRED)

message(STATUS "----------------------------------------")
message(STATUS "Compiler   : ${CMAKE_C_COMPILER} (${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION})")
message(STATUS "- CFLAGS   : ${CMAKE_C_FLAGS}")
message(STATUS "Paths:")
message(STATUS "- prefix   : ${CMAKE_INSTALL_PREFIX}")
message(STATUS "- configs  : ${CMAKE_INSTALL_FULL_SYSCONFDIR}")
message(STATUS "- binary   : ${CMAKE_INSTALL_FULL_BINDIR}")
message(STATUS "- binary   : ${CMAKE_INSTALL_FULL_SBINDIR}")
message(STATUS "- plugins  : ${CMAKE_INSTALL_FULL_LIBDIR}/${CNAME}")
message(STATUS "- data     : ${CMAKE_INSTALL_FULL_DATAROOTDIR}")
message(STATUS "- state    : ${CMAKE_INSTALL_FULL_LOCALSTATEDIR}")
message(STATUS "Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "Options:")
message(STATUS "- WITH_CLIENT    : ${WITH_CLIENT}")
message(STATUS "- WITH_CSOCKET   : ${WITH_CSOCKET}")
message(STATUS "- WITH_HARDENING : ${WITH_HARDENING}")
message(STATUS "- WITH_PCRE      : ${WITH_PCRE}")
message(STATUS "- WITH_REDIS     : ${WITH_REDIS}")
message(STATUS "- WITH_IPSET     : ${WITH_IPSET}")
message(STATUS "- WITH_READLINE  : ${WITH_READLINE}")
message(STATUS "Components:")

add_subdirectory(src)
add_subdirectory(t)
set_property(DIRECTORY "t" PROPERTY COMPILE_FLAGS "-g;-ggdb;-Wall;-Wextra;-pedantic;-O0")

file(GLOB_RECURSE CONFIGS "*.conf.in")
foreach(CONFIG ${CONFIGS})
  string(REPLACE ".conf.in" ".conf" GENERATED ${CONFIG})
  configure_file(${CONFIG} ${GENERATED})
endforeach()
install(DIRECTORY "configs/"     DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/${CNAME}"
  FILES_MATCHING PATTERN "*.conf")
install(DIRECTORY "configs/"     DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/${CNAME}"
  FILES_MATCHING PATTERN "README.txt")
install(FILES "configs/f2b.conf" DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/${CNAME}"
  RENAME "f2b.conf.sample")

if (INIT_SCRIPT STREQUAL "openrc")
  install(FILES "contrib/init.openrc" DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/init.d" RENAME "f2b")
elseif (INIT_SCRIPT STREQUAL "systemd")
  install(FILES "contrib/f2b.service" DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/systemd/system")
endif ()

add_custom_target("dist" COMMAND
  "git" "archive" "--format=tar.gz"
  "--output=${CNAME}_v${VERSION}.tar.gz"
  "--prefix=${CNAME}_v${VERSION}/" "v${VERSION}")

add_custom_target("dist-latest" COMMAND
  "git" "archive" "--format=tar.gz"
  "--output=${CNAME}_latest.tar.gz"
  "--prefix=${CNAME}_latest/" "master")

add_custom_target("lint" COMMAND "cppcheck" "src/")
