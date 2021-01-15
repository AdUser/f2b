#ifndef F2B_BUF_H_
#define F2B_BUF_H_

typedef struct f2b_buf_t {
  size_t used; /**< bytes used in buffer */
  size_t size; /**< available size in data */
  char *data;  /**< allocated buffer */
} f2b_buf_t;

bool   f2b_buf_alloc(f2b_buf_t *buf, size_t max);
void   f2b_buf_free(f2b_buf_t *buf);
bool   f2b_buf_append(f2b_buf_t *buf, const char *str, size_t size);
char * f2b_buf_extract(f2b_buf_t *buf, const char *end);
size_t f2b_buf_splice(f2b_buf_t *buf, size_t len);

#endif /* F2B_BUF_H_ */
