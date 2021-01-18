/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <limits.h>
#include <sys/file.h>
#include <sys/stat.h>

#include <glob.h>

#include "source.h"
#define MODNAME "files"

typedef struct f2b_file_t {
  struct f2b_file_t *next;
  bool opened;
  char path[PATH_MAX];
  FILE *fd;
  struct stat st;
} f2b_file_t;

struct _config {
  char path[256];
  void (*logcb)(enum loglevel lvl, const char *msg);
  f2b_file_t *files;
  f2b_file_t *current;
};

#include "source.c"

static bool
file_open(f2b_file_t *file, const char *path) {
  struct stat st;
  char buf[PATH_MAX] = "";

  assert(file != NULL);
  assert(path != NULL || file->path[0] != '\0');

  strlcpy(buf, path ? path : file->path, sizeof(buf));

  memset(file, 0x0, sizeof(f2b_file_t));

  if (stat(buf, &st) != 0)
    return false;

  if (!(S_ISREG(st.st_mode) || S_ISFIFO(st.st_mode)))
    return false;

  if ((file->fd = fopen(buf, "r")) == NULL)
    return false;

  if (S_ISREG(st.st_mode) && fseek(file->fd, 0, SEEK_END) < 0)
    return false;

  memcpy(&file->st, &st, sizeof(st));
  strlcpy(file->path, buf, sizeof(file->path));
  file->opened = true;

  return true;
}

static void
file_close(f2b_file_t *file) {
  assert(file != NULL);

  if (file->fd)
    fclose(file->fd);

  file->opened = false;
  file->fd = NULL;
}

static bool
file_rotated(const cfg_t *cfg, const f2b_file_t *file) {
  struct stat st;
  assert(file != NULL);

  if (!file->opened)
    return true;

  if (stat(file->path, &st) != 0) {
    log_msg(cfg, error, "file stat error: %s", strerror(errno));
    return true;
  }

  if (file->st.st_dev  != st.st_dev ||
      file->st.st_ino  != st.st_ino ||
      file->st.st_size  > st.st_size) {
    log_msg(cfg, info, "file replaced: %s", file->path);
    return true;
  }

  return false;
}

static bool
file_getline(const f2b_file_t *file, char *buf, size_t bufsize) {
  assert(file != NULL);
  assert(buf != NULL);

  if (feof(file->fd))
    clearerr(file->fd);
  /* fread()+EOF set is implementation defined */
  if (fgets(buf, bufsize, file->fd) != NULL)
    return true;

  return false;
}

cfg_t *
create(const char *init) {
  cfg_t *cfg = NULL;
  assert(init != NULL);
  if ((cfg = calloc(1, sizeof(cfg_t))) == NULL)
    return NULL;
  strlcpy(cfg->path, init, sizeof(cfg->path));
  cfg->logcb = &logcb_stub;
  return cfg;
}

bool
config(cfg_t *cfg, const char *key, const char *value) {
  assert(cfg != NULL);
  assert(key   != NULL);
  assert(value != NULL);

  /* no options */
  (void)(cfg);   /* suppress warning for unused variable 'ip' */
  (void)(key);   /* suppress warning for unused variable 'ip' */
  (void)(value); /* suppress warning for unused variable 'ip' */

  return false;
}

bool
ready(cfg_t *cfg) {
  assert(cfg != NULL);
  if (cfg->path[0] == '\0')
    return false;
  return true;
}

bool
start(cfg_t *cfg) {
  f2b_file_t *file = NULL;
  glob_t globbuf;

  assert(cfg != NULL);

  if (glob(cfg->path, GLOB_MARK | GLOB_NOESCAPE, NULL, &globbuf) != 0)
    return NULL;

  for (size_t i = 0; i < globbuf.gl_pathc; i++) {
    if ((file = calloc(1, sizeof(f2b_file_t))) == NULL)
      continue;
    if (file_open(file, globbuf.gl_pathv[i]) == false) {
      log_msg(cfg, error, "can't open file: %s -- %s", globbuf.gl_pathv[i], strerror(errno));
      free(file);
      continue;
    }
    if (cfg->files == NULL) {
      cfg->files = file;
    } else {
      file->next = cfg->files;
      cfg->files = file;
    }
  }

  globfree(&globbuf);
  return true;
}

bool
stop(cfg_t *cfg) {
  f2b_file_t *next = NULL;

  assert(cfg != NULL);

  for (; cfg->files != NULL; cfg->files = next) {
    next = cfg->files->next;
    file_close(cfg->files);
    free(cfg->files);
  }

  return true;
}

bool
next(cfg_t *cfg, char *buf, size_t bufsize, bool reset) {
  assert(cfg != NULL);
  assert(buf != NULL);
  assert(bufsize > 0);

  if (reset || cfg->current == NULL)
    cfg->current = cfg->files;

  for (f2b_file_t *file = cfg->current; file != NULL; file = file->next) {
    if (file_rotated(cfg, file))
      file_close(file);
    if (!file->opened && !file_open(file, NULL)) {
      log_msg(cfg, error, "can't open file: %s", file->path);
      continue;
    }
    if (file_getline(file, buf, bufsize))
      return true;
  }

  return false;
}

void
destroy(cfg_t *cfg) {
  assert(cfg != NULL);

  free(cfg);
}
