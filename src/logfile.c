/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "logfile.h"

bool
f2b_logfile_open(f2b_logfile_t *file, const char *path) {
  struct stat st;
  char buf[PATH_MAX] = "";

  assert(file != NULL);
  assert(path != NULL || file->path[0] != '\0');

  strlcpy(buf, path ? path : file->path, sizeof(buf));

  memset(file, 0x0, sizeof(f2b_logfile_t));

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

void
f2b_logfile_close(f2b_logfile_t *file) {
  assert(file != NULL);

  if (file->fd)
    fclose(file->fd);

  file->opened = false;
  file->fd = NULL;
}

bool
f2b_logfile_rotated(const f2b_logfile_t *file) {
  struct stat st;

  assert(file != NULL);

  if (stat(file->path, &st) != 0)
    return true;

  if (file->st.st_dev  != st.st_dev ||
      file->st.st_ino  != st.st_ino ||
      file->st.st_size  > st.st_size)
    return true;

  return false;
}

bool
f2b_logfile_getline(const f2b_logfile_t *file, char *buf, size_t bufsize) {
  if (fgets(buf, bufsize, file->fd) != NULL)
    return true;

  return false;
}
