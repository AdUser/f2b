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
      FREE(file);
      continue;
    }
    files = f2b_filelist_append(files, file);
  }

  globfree(&globbuf);
  return files;
}
