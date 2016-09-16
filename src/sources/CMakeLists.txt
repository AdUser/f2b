set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(SOURCES "")

add_library("f2b_source_files" MODULE "files.c" "../strlcpy.c")
list(APPEND SOURCES "f2b_source_files")

message(STATUS "- Sources  : ${SOURCES}")

install(TARGETS ${SOURCES}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
