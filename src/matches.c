/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "matches.h"

f2b_match_t *
f2b_match_create(time_t t) {
  f2b_match_t *m;

  if ((m = calloc(1, sizeof(f2b_match_t))) == NULL)
    return false;
  m->time = t;
  return m;
}

void
f2b_matches_flush(f2b_matches_t *ms) {
  f2b_match_t *head, *tmp;
  assert(ms != NULL);

  head = ms->list;
  while (head != NULL) {
    tmp = head;
    head = head->next;
    free(tmp);
  }
  memset(ms, 0x0, sizeof(f2b_matches_t));
}

void
f2b_matches_append(f2b_matches_t *ms, f2b_match_t *m) {
  assert(ms != NULL);
  assert(m != NULL);

  m->next = ms->list;
  ms->list = m;
  ms->count++;
  ms->last = m->time > ms->last ? m->time : ms->last;
}

void
f2b_matches_expire(f2b_matches_t *ms, time_t before) {
  f2b_match_t *prev, *node, *next;
  assert(ms != NULL);

  if (ms->list == NULL)
    return;

  node =  ms->list;
  prev = NULL;
  while (node != NULL) {
    if (node->time > before) {
      prev = node; node = node->next;
      continue;
    }
    next = node->next;
    free(node);
    if (prev) {
      prev->next = next;
    } else {
      ms->list = next;
    }
    node = next;
    ms->count--;
  }
  ms->last = ms->list ? ms->list->time : 0;
}
