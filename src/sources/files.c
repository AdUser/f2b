/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "source.h"

#include <limits.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <glob.h>
#include <time.h>

#define MODNAME "files"

typedef struct f2b_file_t {
  struct f2b_file_t *next;
  uint32_t lines;
  uint32_t stag;
  FILE *fd;
  char path[PATH_MAX];
  struct stat st;
} f2b_file_t;

struct _config {
  void (*logcb)(enum loglevel lvl, const char *msg);
  f2b_file_t *files;
  f2b_file_t *current;
  int flags;
  char path[256];
};

#include "source.c"

static bool
file_open(f2b_file_t *file, const char *path) {
  FILE *fd;
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

  if ((fd = fopen(buf, "r")) == NULL)
    return false;

  if (S_ISREG(st.st_mode) && fseek(fd, 0, SEEK_END) < 0) {
    fclose(fd);
    return false;
  }

  memcpy(&file->st, &st, sizeof(st));
  strlcpy(file->path, buf, sizeof(file->path));
  file->fd = fd;
  file->stag = fnv_32a_str(file->path, FNV1_32A_INIT);

  return true;
}

static void
file_close(f2b_file_t *file) {
  assert(file != NULL);

  if (file->fd == NULL)
    return;

  fclose(file->fd);
  file->fd = NULL;
  file->lines = 0;
  memset(&file->st, 0, sizeof(struct stat));
}

static bool
file_rotated(const cfg_t *cfg, f2b_file_t *file) {
  struct stat st;
  assert(file != NULL);

  if (file->fd == NULL)
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
  } else if (file->st.st_size < st.st_size) {
    memcpy(&file->st, &st, sizeof(struct stat));
  }

  return false;
}

static bool
file_getline(const cfg_t *cfg, f2b_file_t *file, char *buf, size_t bufsize) {
  char *p;
  assert(file != NULL);
  assert(buf != NULL);

  /* fread()+EOF set is implementation defined */
  if (fgets(buf, bufsize, file->fd) != NULL) {
    if ((p = strchr(buf, '\n')) != NULL)
      *p = '\0'; /* strip newline(s) */
    file->lines++;
    return true;
  }
  if (feof(file->fd))
    clearerr(file->fd);
  if (ferror(file->fd)) {
    log_msg(cfg, error, "file error: %s -- %s", file->path, strerror(errno));
    clearerr(file->fd);
  }

  return false;
}

cfg_t *
create(const char *init) {
  cfg_t *cfg = NULL;
  if ((cfg = calloc(1, sizeof(cfg_t))) == NULL)
    return NULL;
  cfg->logcb = &logcb_stub;
  cfg->flags |= MOD_TYPE_SOURCE;
  cfg->flags |= MOD_NEED_FILTER;
  if (init != NULL && strlen(init) > 0) {
    strlcpy(cfg->path, init, sizeof(cfg->path));
    cfg->flags |= MOD_IS_READY;
  }
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

uint32_t
next(cfg_t *cfg, char *buf, size_t bufsize, bool reset) {
  assert(cfg != NULL);
  assert(buf != NULL);
  assert(bufsize > 0);

  if (reset || cfg->current == NULL)
    cfg->current = cfg->files;

  for (f2b_file_t *file = cfg->current; file != NULL; file = file->next) {
    if (file_rotated(cfg, file))
      file_close(file);
    if (file->fd == NULL && !file_open(file, NULL)) {
      log_msg(cfg, error, "can't open file: %s", file->path);
      continue;
    }
    if (file_getline(cfg, file, buf, bufsize))
      return file->stag;
  }

  return 0;
}

bool
stats(cfg_t *cfg, char *buf, size_t bufsize) {
  struct tm tm;
  char tmp[PATH_MAX + 512];
  char mtime[30];
  const char *fmt =
    "- path: %s\n"
    "  mtime: %s\n"
    "  file: fd=%d inode=%d size=%ld pos=%ld tag=%08X\n"
    "  read: %lu lines\n";
  int fd, ino; off_t sz; long pos;
  assert(cfg != NULL);
  assert(buf != NULL);
  assert(bufsize > 0);

  if (buf == NULL || bufsize == 0)
    return false;

  for (f2b_file_t *f = cfg->files; f != NULL; f = f->next) {
    if (f->fd) {
      fd = fileno(f->fd), ino = f->st.st_ino, sz = f->st.st_size, pos = ftell(f->fd);
      localtime_r(&f->st.st_mtime, &tm);
      strftime(mtime, sizeof(mtime), "%Y-%m-%d %H:%M:%S", &tm);
    } else {
      fd = -1, ino = -1, sz = 0, pos = -1;
      strlcpy(mtime, "-", sizeof(mtime));
    }
    snprintf(tmp, sizeof(tmp), fmt, f->path, mtime, fd, ino, sz, pos, f->stag, f->lines);
    strlcat(buf, tmp, bufsize);
  }

  return true;
}

void
destroy(cfg_t *cfg) {
  assert(cfg != NULL);

  free(cfg);
}
