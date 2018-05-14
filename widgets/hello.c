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

#include "widgets.h"
#include "log.h"

void init_hello(void)
{
	LOGI("Init hello");
}

void draw_hello(xcb_window_t win, int *pos)
{
	*pos = 4;
	LOGI("Draw hello: %d", win);
}

void exit_hello(void)
{
	LOGI("Exit hello");
}

struct widget_module_t WIDGET_SYMBOL = {
	.name = "hello",
	.width = 4,
	.init = init_hello,
	.draw = draw_hello,
	.exit = exit_hello,
};
