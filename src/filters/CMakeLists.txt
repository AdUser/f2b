set(CMAKE_INCLUDE_CURRENT_DIR ON)
unset(CMAKE_SHARED_MODULE_PREFIX)
set(FILTERS "")

add_library("f_preg" MODULE "preg.c" "../strlcpy.c")
list(APPEND FILTERS "preg")

find_library(PCRE_FOUND "pcre")
if (WITH_PCRE AND PCRE_FOUND)
  add_library("f_pcre" MODULE "pcre.c" "../strlcpy.c")
  target_link_libraries("f_pcre" "pcre")
  list(APPEND FILTERS "pcre")
endif ()

foreach (FILTER IN LISTS FILTERS)
  set_target_properties("f_${FILTER}" PROPERTIES OUTPUT_NAME "filter_${FILTER}")
  install(TARGETS "f_${FILTER}" LIBRARY DESTINATION ${PLUGINS_PATH})
endforeach ()

message(STATUS "- Filters  : ${FILTERS}")
