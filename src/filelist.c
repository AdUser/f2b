/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <glob.h>

#include "common.h"
#include "logfile.h"
#include "filelist.h"
#include "log.h"

f2b_logfile_t *
f2b_filelist_append(f2b_logfile_t *list, f2b_logfile_t *file) {
  assert(file != NULL);

  if (list != NULL)
    return file->next = list;
  return file;
}

f2b_logfile_t *
f2b_filelist_from_glob(const char *pattern) {
  f2b_logfile_t *file = NULL;
  f2b_logfile_t *files = NULL;
  glob_t globbuf;

  assert(pattern != NULL);

  if (glob(pattern, GLOB_MARK | GLOB_NOESCAPE, NULL, &globbuf) != 0)
    return NULL;

  for (size_t i = 0; i < globbuf.gl_pathc; i++) {
    if ((file = calloc(1, sizeof(f2b_logfile_t))) == NULL)
      continue;
    if (f2b_logfile_open(file, globbuf.gl_pathv[i]) == false) {
      f2b_log_msg(log_error, "can't open file: %s: %s", globbuf.gl_pathv[i], strerror(errno));
      free(file);
      continue;
    }
    files = f2b_filelist_append(files, file);
  }

  globfree(&globbuf);
  return files;
}

void
f2b_filelist_destroy(f2b_logfile_t *list) {
  f2b_logfile_t *next = NULL;

  for (; list != NULL; list = next) {
    next = list->next;
    f2b_logfile_close(list);
    free(list);
  }
}
