/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

static void
logcb_stub(enum loglevel lvl, const char *str) {
  assert(str != NULL);
  (void)(lvl);
  (void)(str);
}

__attribute__ ((format (printf, 3, 4)))
static void
log_msg(const cfg_t *cfg, enum loglevel lvl, const char *format, ...) {
  char buf[4096] = "";
  va_list args;
  size_t len;

  len = snprintf(buf, sizeof(buf), "filter/%s: ", MODNAME);
  va_start(args, format);
  vsnprintf(buf + len, sizeof(buf) - len, format, args);
  va_end(args);

  cfg->logcb(lvl, buf);
}

void
logcb(cfg_t *cfg, void (*cb)(enum loglevel lvl, const char *msg)) {
  assert(cfg != NULL);
  assert(cb  != NULL);

  cfg->logcb = cb;
}

bool
stats(cfg_t *cfg, char *buf, size_t bufsize) {
  char tmp[PATTERN_MAX + 64];
  const char *fmt =
    "- pattern: %s\n"
    "  matches: %d\n";

  assert(cfg != NULL);

  if (buf == NULL || bufsize == 0)
    return false;

  for (rx_t *rx = cfg->regexps; rx != NULL; rx = rx->next) {
    snprintf(tmp, sizeof(tmp), fmt, rx->pattern, rx->matches);
    strlcat(buf, tmp, bufsize);
  }

  return true;
}

