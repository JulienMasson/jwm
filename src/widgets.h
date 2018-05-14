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

#ifndef WIDGETS_H
#define WIDGETS_H

#include <stdbool.h>

#include <xcb/xcb.h>

#define WIDGET_SYMBOL widget
#define WIDGET_SYMBOL_STR "widget"

struct widget_module_t {
	const char *name;
	int width;
	void (*init)(void);
	void (*draw)(xcb_window_t win, int *pos);
	void (*exit)(void);
};

void widgets_init(int height);

int widgets_get_width(void);

xcb_window_t widgets_get_win(void);

void widgets_toggle(bool enable);

void widgets_reload(void);

#endif
