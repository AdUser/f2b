/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_MATCHES_H_
#define F2B_MATCHES_H_

typedef struct {
  size_t max;
  size_t used;
  time_t *times;
} f2b_matches_t;

bool f2b_matches_create (f2b_matches_t *m, size_t max);
void f2b_matches_destroy(f2b_matches_t *m);

bool f2b_matches_append (f2b_matches_t *m, time_t t);
void f2b_matches_expire (f2b_matches_t *m, time_t t);

#endif /* F2B_MATCHES_H_ */
