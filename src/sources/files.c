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

typedef struct f2b_file_t {
  struct f2b_file_t *next;
  bool opened;
  char path[PATH_MAX];
  FILE *fd;
  struct stat st;
} f2b_file_t;

struct _config {
  char error[256];
  f2b_file_t *files;
};

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
file_rotated(const f2b_file_t *file) {
  struct stat st;

  assert(file != NULL);

  if (!file->opened)
    return true;

  if (stat(file->path, &st) != 0)
    return true;

  if (file->st.st_dev  != st.st_dev ||
      file->st.st_ino  != st.st_ino ||
      file->st.st_size  > st.st_size)
    return true;

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

static f2b_file_t *
list_append(f2b_file_t *list, f2b_file_t *file) {
  assert(file != NULL);

  if (list != NULL)
    return file->next = list;
  return file;
}

static f2b_file_t *
list_from_glob(const char *pattern) {
  f2b_file_t *file = NULL;
  f2b_file_t *files = NULL;
  glob_t globbuf;

  assert(pattern != NULL);

  if (glob(pattern, GLOB_MARK | GLOB_NOESCAPE, NULL, &globbuf) != 0)
    return NULL;

  for (size_t i = 0; i < globbuf.gl_pathc; i++) {
    if ((file = calloc(1, sizeof(f2b_file_t))) == NULL)
      continue;
    if (file_open(file, globbuf.gl_pathv[i]) == false) {
      /* f2b_log_msg(log_error, "can't open file: %s: %s", globbuf.gl_pathv[i], strerror(errno)); */
      free(file);
      continue;
    }
    files = list_append(files, file);
  }

  globfree(&globbuf);
  return files;
}

static void
list_destroy(f2b_file_t *list) {
  f2b_file_t *next = NULL;

  for (; list != NULL; list = next) {
    next = list->next;
    file_close(list);
    free(list);
  }
}
