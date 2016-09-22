set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(SOURCES "")

add_library("f2b_source_files" MODULE "files.c" "../strlcpy.c")
list(APPEND SOURCES "f2b_source_files")

find_library(REDIS_FOUND "hiredis")
if (WITH_REDIS AND REDIS_FOUND)
  add_library("f2b_source_redis" MODULE "redis.c" "../strlcpy.c")
  target_link_libraries("f2b_source_redis" "hiredis")
  list(APPEND SOURCES "f2b_source_redis")
endif ()

message(STATUS "- Sources  : ${SOURCES}")

install(TARGETS ${SOURCES}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
