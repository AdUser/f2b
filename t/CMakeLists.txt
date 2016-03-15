enable_testing()

set(SRC_DIR "../src")

add_executable("t_logfile" "t_logfile.c" "${SRC_DIR}/logfile.c")
add_executable("t_matches" "t_matches.c" "${SRC_DIR}/matches.c")
add_executable("t_ipaddr"  "t_ipaddr.c"  "${SRC_DIR}/ipaddr.c" "${SRC_DIR}/matches.c")
add_executable("t_config_param" "t_config_param.c" "${SRC_DIR}/config.c" "${SRC_DIR}/log.c")

add_test("tests/f2b_logfile_*" "t_logfile")
add_test("tests/f2b_matches_*" "t_matches")
add_test("tests/f2b_ipaddr_*"  "t_ipaddr")
add_test("tests/f2b_config_param*" "t_config_param")

add_executable("t_filter_preg" "t_filters.c" "${SRC_DIR}/filters/preg.c")
add_test("tests/filter/preg"   "t_filter_preg")

if (WITH_PCRE)
add_test("tests/filter/pcre"   "t_filter_pcre")
add_executable("t_filter_pcre" "t_filters.c" "${SRC_DIR}/filters/pcre.c")
target_link_libraries("t_filter_pcre" "pcre")
endif ()
