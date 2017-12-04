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

#ifndef MONITOR_H
#define MONITOR_H

#include <xcb/randr.h>
#include "list.h"

struct monitor {
	xcb_randr_output_t	 id;
	char			*name;
	int16_t			 y, x;	        /* X and Y */
	uint16_t		 width, height;	/* Width/Height in pixels */
	struct list		*element;	/* Pointer to our place in output list */
};

void monitor_init(void);

struct monitor *monitor_find_by_coord(const int16_t x, const int16_t y);

void monitor_event(uint8_t response_type);

void monitor_borders(uint16_t *border_x, uint16_t *border_y);

#endif
