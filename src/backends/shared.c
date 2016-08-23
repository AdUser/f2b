/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

typedef struct cfg_id_t {
  struct cfg_id_t *next;
  char name[ID_MAX + 1];
  size_t count;
} cfg_id_t;

/* this list needed for tracking backend usage with `shared = yes` */
cfg_id_t *ids_usage = NULL;

static size_t
usage_inc(const char *id) {
  cfg_id_t *e = NULL;

  assert(id != NULL);

  for (e = ids_usage; e != NULL; e = e->next) {
    if (strcmp(e->name, id) != 0)
      continue;
    /* found */
    e->count++;
    return e->count;
  }
  /* not found or list is empty */
  e = calloc(1, sizeof(cfg_id_t));
  snprintf(e->name, sizeof(e->name), "%s", id);
  e->count++;
  e->next = ids_usage;
  ids_usage = e;
  return e->count;
}

static size_t
usage_dec(const char *id) {
  cfg_id_t *e = NULL;

  assert(id != NULL);

  for (e = ids_usage; e != NULL; e = e->next) {
    if (strcmp(e->name, id) != 0)
      continue;
    /* found */
    if (e->count > 0)
      e->count--;
    return e->count;
  }

  /* not found or list is empty */
  return 0;
}
