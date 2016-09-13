set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCES "daemon.c" "strlcpy.c" "config.c" "log.c" "matches.c" "ipaddr.c" "source.c" "filter.c" "backend.c" "jail.c")

if (WITH_CSOCKET)
  list(APPEND SOURCES "csocket.c" "cmsg.c")
  add_definitions("-DWITH_CSOCKET")
endif ()

add_executable("f2b" ${SOURCES})

install(TARGETS "f2b" RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR})

if (WITH_CLIENT)
  set(SOURCES "strlcpy.c" "log.c" "client.c" "cmsg.c" "commands.c" "csocket.c")
  add_executable("f2bc" ${SOURCES})
  install(TARGETS "f2bc" RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif ()

set(SOURCES "strlcpy.c" "backend-test.c" "log.c" "config.c" "backend.c")
add_executable("f2b-backend-test" ${SOURCES})

set(SOURCES "strlcpy.c" "filter-test.c" "log.c" "config.c" "filter.c")
add_executable("f2b-filter-test" ${SOURCES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  target_link_libraries(f2b "dl")
  target_link_libraries("f2b-backend-test" "dl")
  target_link_libraries("f2b-filter-test" "dl")
endif ()

install(TARGETS "f2b-filter-test" "f2b-backend-test" RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory("backends")
add_subdirectory("filters")
add_subdirectory("sources")
