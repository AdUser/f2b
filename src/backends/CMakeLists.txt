set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(BACKENDS "")

add_library("f2b_backend_exec" MODULE "exec.c" "../strlcpy.c")
list(APPEND BACKENDS "f2b_backend_exec")

if (WITH_MCAST)
  add_library("f2b_backend_mcast" MODULE "mcast.c" "../strlcpy.c"
    "../commands.c" "../cmsg.c" "../csocket.c")
  list(APPEND BACKENDS "f2b_backend_mcast")
endif ()

find_library(REDIS_FOUND "hiredis")
if (WITH_REDIS AND REDIS_FOUND)
  add_library("f2b_backend_redis" MODULE "redis.c" "../strlcpy.c")
  target_link_libraries("f2b_backend_redis" "hiredis")
  list(APPEND BACKENDS "f2b_backend_redis")
endif ()

find_library(IPSET_FOUND "ipset")
if (WITH_IPSET AND IPSET_FOUND)
  add_library("f2b_backend_ipset" MODULE "ipset.c" "../strlcpy.c")
  target_link_libraries("f2b_backend_ipset" "ipset")
  list(APPEND BACKENDS "f2b_backend_ipset")
endif ()

message(STATUS "- Backends : ${BACKENDS}")

install(TARGETS ${BACKENDS}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
