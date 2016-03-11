set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(FILTERS "")

add_library("f2b_filter_preg" MODULE "preg.c")
list(APPEND FILTERS "f2b_filter_preg")

message(STATUS "- Filters  : ${FILTERS}")

install(TARGETS ${FILTERS}
  LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
