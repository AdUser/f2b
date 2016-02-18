#ifndef F2B_MATCH_H_
#define F2B_MATCH_H_

typedef struct {
  struct f2b_match_t *next;
  const char *ip;
  size_t count;
  time_t firstseen;
  time_t lastseen;
} f2b_match_t;

#endif /* F2B_MATCH_H_ */
