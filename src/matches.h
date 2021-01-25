/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_MATCHES_H_
#define F2B_MATCHES_H_

/**
 * @file
 * This file contains definition of ipaddr matches struct and related routines
 */

typedef struct f2b_match_t {
  struct f2b_match_t *next;
  time_t time;
  uint32_t stag;
  uint32_t ftag;
   int16_t score;
} f2b_match_t;

/** matches container */
typedef struct {
  size_t count;  /**< Count of entries in linked list */
  time_t last;   /**< latest match time */
  f2b_match_t *list; /**< linked list  */
} f2b_matches_t;

/**
 * @brief Allocate memory for new match struct & initialize it
 * @param t Match time
 * @returns pointer to allocated struct
 */
f2b_match_t * f2b_match_create(time_t t);

/**
 * @brief Clean list of matches and free memory
 * @param ms Pointer to struct
 */
void f2b_matches_flush(f2b_matches_t *ms);

/**
 * @brief Insert new match to head of matches list
 * @param ms Matches list struct
 * @param match Match entry
 */
void f2b_matches_prepend (f2b_matches_t *ms, f2b_match_t *m);

/**
 * @brief Remove matches before given time
 * @param m Pointer to struct
 * @param before Start time
 */
void f2b_matches_expire (f2b_matches_t *m, time_t before);

/**
 * @brief Get sum of scores after specified time
 * @param m Pointer to struct
 * @param after Only check matches after this time
 * @returns Sum of scores
 */
int f2b_matches_score(f2b_matches_t *ms, time_t after);

#endif /* F2B_MATCHES_H_ */
