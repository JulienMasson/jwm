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

#include <string.h>

#include "widgets.h"
#include "log.h"

#define HELLO_HEIGHT 30
#define HELLO_WIDTH 39

void init_hello(void)
{
	LOGI("Init hello");
}

void draw_hello(struct draw_t *draw, int *pos)
{
	char *msg = "hello";
	struct area_t area = {*pos, 0, 0, 0};
	int height, width;

	/* get width and height of msg */
	draw_get_text_prop(draw, msg, strlen(msg), &width, &height);
	area.y = (HELLO_HEIGHT - height) / 2;

	/* draw msg */
	draw_set_color(draw, GREY);
	draw_text(draw, msg, strlen(msg), area);

	/* update position */
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
