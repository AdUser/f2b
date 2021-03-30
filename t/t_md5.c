#include "../src/common.h"
#include "../src/md5.h"

int main() {
  MD5_CTX md5;
  char *sample = "098f6bcd4621d373cade4e832627b4f6"; // md5_hex("test")

  memset(&md5, 0x0, sizeof(md5));

  MD5Init(&md5);
  MD5Update(&md5, (unsigned char *) "test", 4);
  MD5Final(&md5);

  assert(strcmp(md5.hexdigest, sample) == 0);
  return 0;
}
