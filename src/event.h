/* Copyright 2020-2021 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_EVENT_H_
#define F2B_EVENT_H_

/**
 * @file
 * This file contains event-related routines
 */

/**
 * @def EVENT_MAX
 * Maximum text length of event line
 */
#define EVENT_MAX 256

/**
 * @brief Send event to all registered handlers
 * @param evt Constant string in specified format
 */
void
f2b_event_send(const char *evt);

/**
 * @brief Adds event handler
 * @param h Pointer to function
 * @note For now you can register not more than three handlers.
 * It will be called in order of registration
 * @return true on success, false on error
 */
bool
f2b_event_handler_register(void (*h)(const char *evt));

/**
 * @brief Clean event handlers list
 */
void
f2b_event_handlers_flush();

#endif /* F2B_EVENT_H_ */
