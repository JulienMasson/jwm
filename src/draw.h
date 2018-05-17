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

#ifndef DRAW_H
#define DRAW_H

#include <stdbool.h>
#include <pango/pangocairo.h>

enum color_t { BLACK, GREY, ORANGE };

struct draw_t {
	cairo_t *cr;
	PangoLayout *layout;
	PangoFontDescription *font;
};

struct area_t {
	double x;
	double y;
	double width;
	double height;
};

/* constructor/destructor */
struct draw_t *draw_create(cairo_surface_t *src, const char *font);
void draw_destroy(struct draw_t *draw);

/* set/get */
void draw_set_font(struct draw_t *draw, const char *font);
void draw_set_color(struct draw_t *draw, enum color_t color);
void draw_get_text_prop(struct draw_t *draw, char *text, size_t len, int *width,
			int *height);

/* draw */
void draw_text(struct draw_t *draw, char *text, size_t len, struct area_t area);
void draw_icon(struct draw_t *draw, const char *path, struct area_t area);
void draw_rectangle(struct draw_t *draw, struct area_t area, bool fill);
void draw_rounded_rectangle(struct draw_t *draw, struct area_t area);

#endif
