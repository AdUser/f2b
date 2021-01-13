#include "common.h"
#include "buf.h"

bool
f2b_buf_alloc(f2b_buf_t *buf, size_t size) {
  assert(buf != NULL);
  assert(size > 0);

  memset(buf, 0x0, sizeof(f2b_buf_t));
  if ((buf->data = malloc(size)) == NULL)
    return false; /* can't allocate memory */

  buf->size = size;
  return true;
}

void
f2b_buf_free(f2b_buf_t *buf) {
  assert(buf != NULL);

  free(buf->data);
  memset(buf, 0x0, sizeof(f2b_buf_t));
}

bool
f2b_buf_append(f2b_buf_t *buf, const char *str, size_t len) {

  assert(buf != NULL);
  assert(str != NULL);

  if (len == 0)
    len = strlen(str);
  if (buf->size < (buf->used + len))
    return false; /* not enough space */

  memcpy(&buf->data[buf->used], str, len);
  buf->used += len;
  buf->data[buf->used] = '\0';
  return true;
}

/**
 * @brief  Extracts line terminated by '\r', '\n'
 * @return Pointer to extracted string on success or NULL otherwise
 * @note Use only with 'read' buffer type
 */
char *
f2b_buf_extract(f2b_buf_t *buf, const char *end) {
  char *s = NULL;
  size_t len = 0;

  assert(buf != NULL);
  assert(end != NULL);

  if (buf->data == NULL || buf->used == 0)
    return NULL; /* no data */

  if ((s = strstr(buf->data, end)) == NULL)
    return NULL; /* not found */

  /* copy the data before modifying buffer */
  len = s - buf->data;
  if ((s = strndup(buf->data, len)) == NULL)
    return NULL; /* malloc error */

  /* shift data inside buffer */
  len += strlen(end);
  assert(buf->used >= len);
  buf->used -= len,
  memmove(buf->data, &buf->data[len], buf->used);
  buf->data[buf->used] = '\0';

  return s;
}
