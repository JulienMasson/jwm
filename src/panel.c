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

#include <cairo/cairo-xcb.h>

#include "global.h"
#include "panel.h"
#include "window.h"
#include "monitor.h"

#define PANEL_REFRESH 60
#define PANEL_HEIGHT 20
#define PANEL_TEXT_SIZE 14

struct panel *panel = NULL;

void panel_init(void)
{
	int16_t border_x, border_y;
	uint16_t border_width, border_height;

	monitor_borders(&border_x, &border_y, &border_width, &border_height);
	panel = malloc(sizeof(struct panel));

	/* init panel values */
	panel->id = window_create(border_x, border_y, border_width, PANEL_HEIGHT);
	panel->x = border_x;
	panel->y = border_y;
	panel->width = border_width;
	panel->height = PANEL_HEIGHT;
	panel->enable = true;
	panel->refresh = PANEL_REFRESH;

	/* show panel */
	window_show(panel->id);
}

struct panel *panel_get(void)
{
	return panel;
}

void panel_update_geom(void)
{
	int16_t border_x, border_y;
	uint16_t border_width, border_height;
	monitor_borders(&border_x, &border_y, &border_width, &border_height);

	if (panel && ((border_x != panel->x) ||
		      (border_y != panel->y) ||
		      (border_width != panel->width))) {
		/* set new values */
		panel->x = border_x;
		panel->y = border_y;
		panel->width = border_width;

		/* move, resize and show panel */
		window_move_resize(panel->id, border_x, border_y, border_width, PANEL_HEIGHT);
		panel_draw();
	}
}

static void draw_clock(cairo_t *cr)
{
	cairo_text_extents_t extents;
	time_t t;
	struct tm *lt;
	char date[30];

	/* get current date */
	t = time(NULL);
	lt = localtime(&t);
	date[strftime(date, sizeof(date), "%R  -  %d %b", lt)] = '\0';

	/* paint it in cairo */
	cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
	cairo_text_extents(cr, date, &extents);
	cairo_move_to(cr, panel->width - extents.width - 5, PANEL_TEXT_SIZE + 3);
	cairo_show_text(cr, date);
}

void panel_draw(void)
{
	cairo_surface_t *src = cairo_xcb_surface_create(conn, panel->id, visual,
							panel->width - panel->x,
							panel->height - panel->y);
	cairo_t *cr = cairo_create(src);
	cairo_select_font_face(cr, "serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(cr, PANEL_TEXT_SIZE);

	/* show "JWM" */
	cairo_set_source_rgb(cr, 0.98, 0.52, 0.07);
	cairo_move_to(cr, 10, PANEL_TEXT_SIZE);
	cairo_show_text(cr, "JWM");

	/* show clock */
	draw_clock(cr);

	/* flush */
	cairo_surface_flush(src);
	xcb_flush(conn);

	/* free resources */
	cairo_surface_destroy(src);
	cairo_destroy(cr);
}

void panel_event(xcb_expose_event_t *ev)
{
	/* only redraw on last expose event */
	if ((ev->count == 0) && (ev->window == panel->id))
		panel_draw();
}
