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

/** matches container */
typedef struct {
  size_t hits;   /**< how many times this ip matched by filter */
  size_t max;    /**< max matches that this in struct can contain */
  size_t used;   /**< currently used matches count */
  time_t *times; /**< dynamic unix timestamps array */
} f2b_matches_t;

/**
 * @brief Init matches struct, allocate memory for @a hits
 * @param m Pointer to struct
 * @param max Max expected matches count
 * @returns true on success, false otherwise
 */
bool f2b_matches_create (f2b_matches_t *m, size_t max);
/**
 * @brief Destroy matches struct and free memory
 * @param m Pointer to struct
 */
void f2b_matches_destroy(f2b_matches_t *m);
/**
 * @brief Push new match time to struct
 * @param m Pointer to struct
 * @param t Match time
 * @returns true on success, false if capacity exceeded
 */
bool f2b_matches_append (f2b_matches_t *m, time_t t);
/**
 * @brief Remove matches before given time
 * @param m Pointer to struct
 * @param t Start time
 */
void f2b_matches_expire (f2b_matches_t *m, time_t t);

#endif /* F2B_MATCHES_H_ */
