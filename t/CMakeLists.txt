enable_testing()

set(SRC_DIR "../src")

add_executable("t_cmd"          "t_cmd.c"     "${SRC_DIR}/strlcpy.c" "${SRC_DIR}/commands.c")
add_executable("t_cmsg"         "t_cmsg.c"    "${SRC_DIR}/strlcpy.c" "${SRC_DIR}/cmsg.c")
add_executable("t_matches"      "t_matches.c" "${SRC_DIR}/strlcpy.c" "${SRC_DIR}/matches.c")
add_executable("t_ipaddr"       "t_ipaddr.c"  "${SRC_DIR}/strlcpy.c" "${SRC_DIR}/matches.c" "${SRC_DIR}/ipaddr.c")
add_executable("t_statefile"    "t_statefile.c"  "${SRC_DIR}/strlcpy.c" "${SRC_DIR}/matches.c" "${SRC_DIR}/ipaddr.c" "${SRC_DIR}/statefile.c" "${SRC_DIR}/log.c")
add_executable("t_config_param" "t_config_param.c" "${SRC_DIR}/strlcpy.c" "${SRC_DIR}/config.c" "${SRC_DIR}/log.c")
add_executable("t_backend_usage" "t_backend_usage.c" "${SRC_DIR}/strlcpy.c")

add_test("tests/f2b_cmd_*"     "t_cmd")
add_test("tests/f2b_cmsg_*"    "t_cmsg")
add_test("tests/f2b_matches_*" "t_matches")
add_test("tests/f2b_ipaddr_*"  "t_ipaddr")
add_test("tests/f2b_statefile_*"  "t_statefile")
add_test("tests/f2b_config_param*" "t_config_param")
add_test("tests/backend/usage_*"   "t_backend_usage")

add_executable("t_filter_preg" "t_filters.c" "${SRC_DIR}/filters/preg.c" "${SRC_DIR}/strlcpy.c")
add_test("tests/filter/preg"   "t_filter_preg")

if (WITH_PCRE)
add_test("tests/filter/pcre"   "t_filter_pcre")
add_executable("t_filter_pcre" "t_filters.c" "${SRC_DIR}/filters/pcre.c" "${SRC_DIR}/strlcpy.c")
target_link_libraries("t_filter_pcre" "pcre")
endif ()
