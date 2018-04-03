/*
 * This file is part of the jwm distribution:
 * https://github.com/JulienMasson/jwm
 *
 * Copyright (c) 2017 Julien Masson.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CLIENT_H
#define CLIENT_H

#include <stdbool.h>

#include "monitor.h"
#include "list.h"

enum client_search_t {
	CLIENT_NEXT,
	CLIENT_PREVIOUS
};

struct sizepos {
	int16_t		x, y;
	uint16_t	width, height;
};

struct client {
	xcb_window_t	id;             // ID of this window.
	int16_t		x, y;           // X/Y coordinate.
	uint16_t	width, height;  // Width,Height in pixels.
	struct sizepos	origsize;       // Original size if we're currently maxed.
	uint16_t	max_width, max_height, min_width, min_height;
	bool		maxed, iconic;
	struct monitor *monitor;        // The physical output this window is on.
	struct list    *index;          // Pointer to our place in global windows list.
};

/* accessors */
void client_foreach(void (*func)(struct client *client, void *data), void *data);
struct client *client_get_focus(void);
struct client *client_get_first(void);
struct client *client_get_circular(struct client *start, enum client_search_t direction);

/* set focus */
void client_set_focus(struct client *client);

/* check client parameters */
void client_check_monitor(struct client *client);
void client_fit_on_screen(struct client *client, void *data);

/* monitor attached to client */
void client_monitor_updated(struct monitor *mon);
void client_monitor_reassign(struct monitor *old, struct monitor *new);

/* events handler */
void client_map_request(xcb_map_request_event_t *ev);
void client_configure_request(xcb_configure_request_event_t *ev);
void client_destroy(xcb_destroy_notify_event_t *ev);
void client_enter(xcb_enter_notify_event_t *ev);
void client_unmap(xcb_unmap_notify_event_t *ev);
void client_message(xcb_client_message_event_t *ev);

#endif
