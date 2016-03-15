/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_REGEX_H_
#define F2B_REGEX_H_

#define REGEX_LINE_MAX 256
#define HOST_TOKEN "<HOST>"

typedef struct _regex f2b_regex_t;

f2b_regex_t * f2b_regex_create (const char *pattern, bool icase);
bool          f2b_regex_match  (f2b_regex_t *regex, const char *line, char *buf, size_t hbuf_size);
void          f2b_regex_destroy(f2b_regex_t *regex);

f2b_regex_t * f2b_regexlist_from_file(const char *path);
f2b_regex_t * f2b_regexlist_append (f2b_regex_t *list, f2b_regex_t *regex);
bool          f2b_regexlist_match  (f2b_regex_t *list, const char *line, char *buf, size_t buf_size);
f2b_regex_t * f2b_regexlist_destroy(f2b_regex_t *list);

#endif /* F2B_REGEX_H_ */
