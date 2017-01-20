set(CMAKE_INCLUDE_CURRENT_DIR ON)
unset(CMAKE_SHARED_MODULE_PREFIX)
set(BACKENDS "")

add_library("b_exec" MODULE "exec.c" "../strlcpy.c")
list(APPEND BACKENDS "exec")

if (WITH_MCAST)
  add_library("b_mcast" MODULE "mcast.c" "../strlcpy.c"
    "../commands.c" "../cmsg.c" "../csocket.c")
  list(APPEND BACKENDS "mcast")
endif ()

find_library(REDIS_FOUND "hiredis")
if (WITH_REDIS AND REDIS_FOUND)
  add_library("b_redis" MODULE "redis.c" "../strlcpy.c")
  target_link_libraries("b_redis" "hiredis")
  list(APPEND BACKENDS "redis")
endif ()

find_library(IPSET_FOUND "ipset")
if (WITH_IPSET AND IPSET_FOUND)
  add_library("b_ipset" MODULE "ipset.c" "../strlcpy.c")
  target_link_libraries("b_ipset" "ipset")
  list(APPEND BACKENDS "ipset")
endif ()

set(LIB_PATH "${CMAKE_INSTALL_LIBDIR}/f2b/backends")
foreach (BACKEND IN LISTS BACKENDS)
  set_target_properties("b_${BACKEND}" PROPERTIES OUTPUT_NAME ${BACKEND})
  install(TARGETS "b_${BACKEND}" LIBRARY DESTINATION ${LIB_PATH})
endforeach ()

message(STATUS "- Backends : ${BACKENDS}")
