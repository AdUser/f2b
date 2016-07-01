set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCES "daemon.c" "strlcpy.c" "logfile.c" "log.c" "matches.c" "ipaddr.c" "filelist.c" "filter.c" "config.c" "jail.c" "backend.c")

if (WITH_CSOCKET)
  list(APPEND SOURCES "csocket.c" "cmsg.c")
  add_definitions("-DWITH_CSOCKET")
endif ()

add_executable("f2b" ${SOURCES})
target_link_libraries(f2b "dl")

install(TARGETS "f2b" RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR})

if (WITH_CLIENT)
  set(SOURCES "strlcpy.c" "log.c" "client.c" "cmsg.c" "commands.c" "csocket.c")
  add_executable("f2bc" ${SOURCES})
  install(TARGETS "f2bc" RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif ()

set(SOURCES "strlcpy.c" "backend-test.c" "log.c" "config.c" "backend.c")
add_executable("backend-test" ${SOURCES})
target_link_libraries("backend-test" "dl")

set(SOURCES "strlcpy.c" "filter-test.c" "log.c" "config.c" "filter.c")
add_executable("filter-test" ${SOURCES})
target_link_libraries("filter-test" "dl")

add_subdirectory("backends")
add_subdirectory("filters")
