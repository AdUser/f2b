set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(BACKENDS "")

add_library("f2b_backend_exec" MODULE "exec.c")
list(APPEND BACKENDS "f2b_backend_exec")

message(STATUS "- Backends : ${BACKENDS}")

install(TARGETS ${BACKENDS}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
