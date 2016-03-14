set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(FILTERS "")

add_library("f2b_filter_preg" MODULE "preg.c")
list(APPEND FILTERS "f2b_filter_preg")

find_library(PCRE_FOUND "pcre")
if (PCRE_FOUND)
  add_library("f2b_filter_pcre" MODULE "pcre.c")
  target_link_libraries("f2b_filter_pcre" "pcre")
  list(APPEND FILTERS "f2b_filter_pcre")
endif ()

message(STATUS "- Filters  : ${FILTERS}")

install(TARGETS ${FILTERS}
  LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
