set(CMAKE_INCLUDE_CURRENT_DIR ON)
unset(CMAKE_SHARED_MODULE_PREFIX)
set(SOURCES "")

add_library("s_files" MODULE "files.c" "../strlcpy.c" "../fnv32a.c")
list(APPEND SOURCES "files")

add_library("s_portknock" MODULE "portknock.c" "../strlcpy.c" "../fnv32a.c")
list(APPEND SOURCES "portknock")

if (WITH_REDIS)
  pkg_check_modules(REDIS "hiredis" REQUIRED)
  add_library("s_redis" MODULE "redis.c" "../strlcpy.c")
  target_link_libraries("s_redis" "hiredis")
  list(APPEND SOURCES "redis")
endif ()

foreach (SOURCE IN LISTS SOURCES)
  set_target_properties("s_${SOURCE}" PROPERTIES OUTPUT_NAME "source_${SOURCE}")
  install(TARGETS "s_${SOURCE}" LIBRARY DESTINATION ${PLUGINS_PATH})
endforeach ()

message(STATUS "- Sources  : ${SOURCES}")
