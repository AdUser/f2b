set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCES "main.c" "strlfuncs.c" "logfile.c" "log.c" "matches.c" "ipaddr.c" "filelist.c" "filter.c" "config.c" "jail.c" "backend.c")

add_executable("f2b" ${SOURCES})
target_link_libraries(f2b "dl")

set(SOURCES "strlfuncs.c" "backend-test.c" "log.c" "config.c" "backend.c")
add_executable("backend-test" ${SOURCES})
target_link_libraries("backend-test" "dl")

set(SOURCES "strlfuncs.c" "filter-test.c" "log.c" "config.c" "filter.c")
add_executable("filter-test" ${SOURCES})
target_link_libraries("filter-test" "dl")

install(TARGETS f2b
        RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")

add_subdirectory("backends")
add_subdirectory("filters")
