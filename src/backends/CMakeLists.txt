set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(BACKENDS "")

add_library("f2b_backend_exec" MODULE "exec.c")
list(APPEND BACKENDS "f2b_backend_exec")

find_library(REDIS_FOUND "pcre")
if (WITH_REDIS AND REDIS_FOUND)
  add_library("f2b_backend_redis" MODULE "redis.c")
  target_link_libraries("f2b_backend_redis" "hiredis")
  list(APPEND BACKENDS "f2b_backend_redis")
endif ()

message(STATUS "- Backends : ${BACKENDS}")

install(TARGETS ${BACKENDS}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
