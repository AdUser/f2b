enable_testing()

set(SRC_DIR "../src")

add_executable("t_logfile" "t_logfile.c" "${SRC_DIR}/logfile.c")

add_test("tests/f2b_logfile_*" "t_logfile")
