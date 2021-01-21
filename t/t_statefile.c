#include "../src/common.h"
#include "../src/matches.h"
#include "../src/ipaddr.h"
#include "../src/statefile.h"

int main() {
  bool res = false;
  f2b_ipaddr_t *list = NULL;
  f2b_statefile_t *sf = NULL;
  char jailname[64] = "";
  struct stat st;
  time_t banned_at = 0, release_at = 0;

  UNUSED(res);
  UNUSED(list);

  snprintf(jailname, sizeof(jailname), "%lu", time(NULL)); /* gen unique jailname */

  sf = f2b_statefile_create("/tmp", jailname);
  assert(sf != NULL);

  list = f2b_statefile_load(sf);
  assert(list == NULL);

  banned_at = time(NULL) - 60;
  release_at = banned_at + 60 * 2;

  list = f2b_ipaddr_create("1.2.3.4");
  list->banned = true;
  list->banned_at  = banned_at;
  list->release_at = release_at;

  /* empty file should be created after f2b_statefile_create() call */
  stat(sf->path, &st);
  assert(st.st_size == 0);

  res = f2b_statefile_save(sf, list);
  assert(res == true);

  stat(sf->path, &st);
  assert(st.st_size > 0);

  f2b_ipaddr_destroy(list);
  list = NULL;

  list = f2b_statefile_load(sf);
  assert(list != NULL);
  assert(list->banned     == true);
  assert(list->banned_at  == banned_at);
  assert(list->release_at == release_at);

  f2b_ipaddr_destroy(list);

  unlink(sf->path);

  f2b_statefile_destroy(sf);

  return 0;
}
