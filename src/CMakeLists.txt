set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCES "main.c" "logfile.c" "log.c" "matches.c" "ipaddr.c" "filelist.c" "regexps_posix.c" "config.c" "jail.c")

add_executable(f2b ${SOURCES})

install(TARGETS f2b
        RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
