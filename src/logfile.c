/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "logfile.h"

bool
f2b_logfile_open(f2b_logfile_t *file, const char *filename) {
  struct stat st;

  assert(file != NULL);
  assert(filename != NULL);

  memset(file, 0x0, sizeof(f2b_logfile_t));

  if (stat(filename, &st) != 0)
    return false;

  if (!(S_ISREG(st.st_mode) || S_ISFIFO(st.st_mode)))
    return false;

  strlcpy(file->path, filename, sizeof(file->path));
  memcpy(&file->st, &st, sizeof(st));

  if ((file->fd = fopen(filename, "r")) == NULL)
    return false;

  if (S_ISREG(st.st_mode) && fseek(file->fd, 0, SEEK_END) < 0)
    return false;

  return true;
}

void
f2b_logfile_close(const f2b_logfile_t *file) {
  assert(file != NULL);
  fclose(file->fd);
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
