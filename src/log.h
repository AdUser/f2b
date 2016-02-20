#ifndef F2B_LOG_H_
#define F2B_LOG_H_

typedef enum {
  log_debug = 0,
  log_info  = 1,
  log_warn  = 2,
  log_error = 3,
  log_fatal = 4
} log_msgtype_t;

void log_msg (log_msgtype_t l, const char *fmt, ...);

#endif /* F2B_LOG_H_ */
