set(CMAKE_INCLUDE_CURRENT_DIR ON)
unset(CMAKE_SHARED_MODULE_PREFIX)
set(SOURCES "")

add_library("s_files" MODULE "files.c" "../strlcpy.c")
list(APPEND SOURCES "files")

add_library("s_portknock" MODULE "portknock.c" "../strlcpy.c")
list(APPEND SOURCES "portknock")

if (WITH_MCAST)
  add_library("s_mcast" MODULE "mcast.c" "../strlcpy.c"
    "../commands.c" "../cmsg.c" "../csocket.c")
  list(APPEND SOURCES "mcast")
endif ()

find_library(REDIS_FOUND "hiredis")
if (WITH_REDIS AND REDIS_FOUND)
  add_library("s_redis" MODULE "redis.c" "../strlcpy.c")
  target_link_libraries("s_redis" "hiredis")
  list(APPEND SOURCES "redis")
endif ()

foreach (SOURCE IN LISTS SOURCES)
  set_target_properties("s_${SOURCE}" PROPERTIES OUTPUT_NAME "source_${SOURCE}")
  install(TARGETS "s_${SOURCE}" LIBRARY DESTINATION ${PLUGINS_PATH})
endforeach ()

message(STATUS "- Sources  : ${SOURCES}")
