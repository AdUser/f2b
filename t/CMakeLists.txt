enable_testing()

set(SRC_DIR "../src")

add_executable("t_logfile" "t_logfile.c" "${SRC_DIR}/logfile.c")
add_executable("t_matches" "t_matches.c" "${SRC_DIR}/matches.c")
add_executable("t_ipaddr"  "t_ipaddr.c"  "${SRC_DIR}/ipaddr.c" "${SRC_DIR}/matches.c")
add_executable("t_rx_posix" "t_rx_posix.c" "${SRC_DIR}/regexps_posix.c")

add_test("tests/f2b_logfile_*" "t_logfile")
add_test("tests/f2b_matches_*" "t_matches")
add_test("tests/f2b_ipaddr_*"  "t_ipaddr")
add_test("tests/f2b_regex_*"   "t_rx_posix")
