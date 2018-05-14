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

#include <pango/pangocairo.h>

#include "widgets.h"
#include "log.h"

#define HELLO_HEIGHT 30
#define HELLO_WIDTH 39

void init_hello(void)
{
	LOGI("Init hello");
}

void draw_hello(cairo_t *cr, int *pos)
{
	PangoLayout *layout;
	PangoFontDescription *font;
	int height, width = HELLO_WIDTH;

	/* init pango */
	layout = pango_cairo_create_layout(cr);
	font = pango_font_description_from_string("sans 12");
	pango_layout_set_font_description(layout, font);

	/* set text and get size */
	pango_layout_set_text(layout, "hello", 5);
	pango_layout_get_pixel_size(layout, NULL, &height);

	/* set color */
	cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
	pango_cairo_update_layout(cr, layout);

	/* set geometry */
	pango_layout_set_width(layout, width * PANGO_SCALE);
	pango_layout_set_height(layout, HELLO_HEIGHT * PANGO_SCALE);
	pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

	/* move and show layout */
	cairo_move_to(cr, *pos, (HELLO_HEIGHT - height) / 2);
	pango_cairo_show_layout(cr, layout);
	g_object_unref(layout);

	*pos += width;
	LOGI("Draw hello: %d", *pos);
}

void exit_hello(void)
{
	LOGI("Exit hello");
}

struct widget_module_t WIDGET_SYMBOL = {
	.name = "hello",
	.width = HELLO_WIDTH,
	.init = init_hello,
	.draw = draw_hello,
	.exit = exit_hello,
};
