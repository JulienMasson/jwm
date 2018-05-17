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

#include <stdlib.h>
#include <math.h>

#include "draw.h"

struct draw_t *draw_create(cairo_surface_t *src, const char *font)
{
	struct draw_t *draw = NULL;

	if ((src == NULL) || (font == NULL))
		return NULL;

	draw = malloc(sizeof(struct draw_t));
	draw->cr = cairo_create(src);
	draw->layout = pango_cairo_create_layout(draw->cr);
	draw_set_font(draw, font);

	return draw;
}

void draw_destroy(struct draw_t *draw)
{
	if (draw == NULL)
		return;

	if (draw->cr != NULL)
		cairo_destroy(draw->cr);

	if (draw->font != NULL)
		pango_font_description_free(draw->font);

	if (draw->layout != NULL)
		g_object_unref(draw->layout);

	free(draw);
}

void draw_set_font(struct draw_t *draw, const char *font)
{
	if ((draw == NULL) || (font == NULL))
		return;

	draw->font = pango_font_description_from_string(font);
	pango_layout_set_font_description(draw->layout, draw->font);
}

void draw_set_color(struct draw_t *draw, enum color_t color)
{
	if ((draw == NULL) || (draw->cr == NULL))
		return;

	switch (color) {
	case BLACK:
		cairo_set_source_rgb(draw->cr, 0, 0, 0);
		break;
	case GREY:
		cairo_set_source_rgb(draw->cr, 0.5, 0.5, 0.5);
		break;
	case ORANGE:
		cairo_set_source_rgb(draw->cr, 0.98, 0.52, 0.07);
		break;
	}
	pango_cairo_update_layout(draw->cr, draw->layout);
}

void draw_get_text_prop(struct draw_t *draw, char *text, size_t len, int *width,
			int *height)
{
	PangoLayout *tmp;

	if ((draw == NULL) || (draw->cr == NULL))
		return;

	tmp = pango_cairo_create_layout(draw->cr);
	pango_layout_set_text(tmp, text, len);
	pango_layout_get_pixel_size(tmp, width, height);

	g_object_unref(tmp);
}

void draw_text(struct draw_t *draw, char *text, size_t len, struct area_t area)
{
	if ((draw == NULL) || (draw->layout == NULL) || (draw->cr == NULL))
		return;

	cairo_save(draw->cr);

	/* set text and get size */
	pango_layout_set_text(draw->layout, text, len);

	/* set geometry */
	pango_layout_set_width(draw->layout, area.width * PANGO_SCALE);
	pango_layout_set_height(draw->layout, area.height * PANGO_SCALE);
	pango_layout_set_alignment(draw->layout, PANGO_ALIGN_CENTER);

	/* move and show layout */
	cairo_move_to(draw->cr, area.x, area.y);
	pango_cairo_show_layout(draw->cr, draw->layout);

	cairo_restore(draw->cr);
}

void draw_icon(struct draw_t *draw, const char *path, struct area_t area)
{
	cairo_surface_t *image;

	if ((draw == NULL) || (draw->cr == NULL))
		return;

	image = cairo_image_surface_create_from_png(path);
	cairo_set_source_surface(draw->cr, image, area.x, area.y);
	cairo_paint(draw->cr);
	cairo_surface_destroy(image);
}

void draw_rectangle(struct draw_t *draw, struct area_t area, bool fill)
{
	if ((draw == NULL) || (draw->cr == NULL))
		return;

	cairo_rectangle(draw->cr, area.x, area.y, area.width, area.height);
	if (fill == true)
		cairo_fill(draw->cr);
}

void draw_rounded_rectangle(struct draw_t *draw, struct area_t area)
{
	double aspect = 1.0;
	double corner_radius = area.height / 10.0;
	double radius = corner_radius / aspect;
	double degrees = M_PI / 180.0;

	if ((draw == NULL) || (draw->cr == NULL))
		return;

	cairo_new_sub_path(draw->cr);
	cairo_arc(draw->cr, area.x + area.width - radius, area.y + radius,
		  radius, -90 * degrees, 0 * degrees);
	cairo_arc(draw->cr, area.x + area.width - radius,
		  area.y + area.height - radius, radius, 0 * degrees,
		  90 * degrees);
	cairo_arc(draw->cr, area.x + radius, area.y + area.height - radius,
		  radius, 90 * degrees, 180 * degrees);
	cairo_arc(draw->cr, area.x + radius, area.y + radius, radius,
		  180 * degrees, 270 * degrees);
	cairo_close_path(draw->cr);

	cairo_set_line_width(draw->cr, 1);
	cairo_stroke(draw->cr);
}
