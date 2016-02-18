set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(f2b "main.c" "logfile.c" "log.c")

install(TARGETS f2b
        RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
