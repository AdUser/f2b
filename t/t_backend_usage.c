#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../src/strlcpy.h"
#include "../src/backends/backend.h"
#include "../src/backends/shared.c"

int main() {
  assert(usage_inc("test1") == 1);
  assert(usage_inc("test1") == 2);
  assert(usage_inc("test2") == 1);
  assert(usage_inc("test1") == 3);

  assert(usage_dec("test1") == 2);
  assert(usage_dec("test1") == 1);
  assert(usage_dec("test1") == 0);
  assert(usage_dec("test1") == 0);

  assert(usage_dec("test3") == 0);

  return 0;
}
