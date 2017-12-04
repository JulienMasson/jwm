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

struct sizepos {
	int16_t		x, y;
	uint16_t	width, height;
};

struct client {                         // Everything we know about a window.
	xcb_drawable_t	id;             // ID of this window.
	bool		usercoord;      // X,Y was set by -geom.
	int16_t		x, y;           // X/Y coordinate.
	uint16_t	width, height;  // Width,Height in pixels.
	struct sizepos	origsize;       // Original size if we're currently maxed.
	uint16_t	max_width, max_height, min_width, min_height;
	bool		fixed, vertmaxed, hormaxed, maxed, verthor, iconic;
	struct monitor *monitor;        // The physical output this window is on.
	struct list *	win;            // Pointer to our place in global windows list.
};

struct client *client_find(const xcb_drawable_t *win);
void client_update_list(void);
void client_destroy(xcb_destroy_notify_event_t *ev);
void client_enter(xcb_enter_notify_event_t *ev);
void client_unmap(xcb_unmap_notify_event_t *ev);
void client_message(xcb_client_message_event_t *ev);

#endif
