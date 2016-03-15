/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef FILELIST_H_
#define FILELIST_H_

f2b_logfile_t *
f2b_filelist_from_glob(const char *pattern);

f2b_logfile_t *
f2b_filelist_append(f2b_logfile_t *list, f2b_logfile_t *file);

void
f2b_filelist_destroy(f2b_logfile_t *list);

#endif /* FILELIST_H_ */
