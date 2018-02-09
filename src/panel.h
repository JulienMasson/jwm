/*
 * This file is part of the jwm distribution:
 * https://github.com/JulienMasson/jwm
 *
 * Copyright (c) 2018 Julien Masson.
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

#ifndef PANEL_H
#define PANEL_H

#include <stdbool.h>
#include <cairo/cairo-xcb.h>
#include <pango/pangocairo.h>

struct panel {
	xcb_window_t		 id;
	int16_t			 x, y;
	uint16_t		 width, height;
	bool			 enable;
	time_t			 refresh;
	cairo_t			*cr;
	cairo_surface_t		*src;
	PangoLayout		*layout;
	PangoFontDescription	*font;
};

void panel_init(void);
struct panel *panel_get(void);
void panel_update_geom(void);
void panel_draw(void);
void panel_event(xcb_expose_event_t *ev);
void panel_add_systray(xcb_client_message_event_t *ev);
void panel_remove_systray(xcb_unmap_notify_event_t *ev);

#endif
