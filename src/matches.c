/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "matches.h"

bool
f2b_matches_create(f2b_matches_t *m, size_t max) {
  assert(m != NULL);
  assert(max != 0);

  if ((m->times = calloc(max, sizeof(time_t))) == NULL)
    return false;

  m->used = 0;
  m->max  = max;
  return true;
}

void
f2b_matches_destroy(f2b_matches_t *m) {
  assert(m != NULL);

  FREE(m->times);
  m->used = 0;
  m->max  = 0;
}

bool
f2b_matches_append(f2b_matches_t *m, time_t t) {
  assert(m != NULL);

  if (m->used >= m->max)
    return false;

  m->times[m->used] = t;
  m->used++;
  return true;
}

void
f2b_matches_expire(f2b_matches_t *m, time_t t) {
  assert(m != NULL);

  for (size_t i = 0; i < m->used; ) {
    if (m->times[i] > t) {
      i++;
      continue;
    }
    m->used--;
    m->times[i] = m->times[m->used];
    m->times[m->used] = 0;
  }
}
