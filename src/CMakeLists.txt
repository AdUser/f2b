set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCES "daemon.c" "strlcpy.c" "config.c" "log.c" "matches.c" "ipaddr.c"
  "appconfig.c" "statefile.c" "source.c" "filter.c" "backend.c" "jail.c")

if (WITH_CSOCKET)
  list(APPEND SOURCES "commands.c" "csocket.c")
  add_definitions("-DWITH_CSOCKET")
endif ()

add_executable("f2b" ${SOURCES})
install(TARGETS "f2b" RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR})

if (WITH_CLIENT)
  set(SOURCES "strlcpy.c" "client.c")
  add_executable("f2bc" ${SOURCES})
  if (WITH_READLINE)
    add_definitions("-DWITH_READLINE")
    target_link_libraries("f2bc" "readline")
  endif ()
  install(TARGETS "f2bc" RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif ()

set(SOURCES "strlcpy.c" "source-test.c" "log.c" "config.c" "source.c")
add_executable("f2b-source-test" ${SOURCES})

set(SOURCES "strlcpy.c" "filter-test.c" "log.c" "config.c" "filter.c")
add_executable("f2b-filter-test" ${SOURCES})

set(SOURCES "strlcpy.c" "backend-test.c" "log.c" "config.c" "backend.c")
add_executable("f2b-backend-test" ${SOURCES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  target_link_libraries("f2b"              "dl")
  target_link_libraries("f2b-source-test"  "dl")
  target_link_libraries("f2b-filter-test"  "dl")
  target_link_libraries("f2b-backend-test" "dl")
endif ()

set_target_properties("f2b" "f2b-source-test" "f2b-filter-test" "f2b-backend-test"
  PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}/${CNAME}")

install(TARGETS "f2b-source-test" "f2b-filter-test" "f2b-backend-test"
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

set(PLUGINS_PATH "${CMAKE_INSTALL_LIBDIR}/${CNAME}")
add_subdirectory("backends")
add_subdirectory("filters")
add_subdirectory("sources")
