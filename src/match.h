#ifndef F2B_MATCH_H_
#define F2B_MATCH_H_

typedef struct {
  struct f2b_match_t *next;
  char ip[40]; /* 8 x "ffff" + 7 x ":" + '\0' */
  time_t seen;
} f2b_match_t;

#endif /* F2B_MATCH_H_ */
