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

#ifndef WINDOW_H
#define WINDOW_H

#include <stdbool.h>

#define WINDOW_BORDER_WIDTH 1
#define WINDOW_BORDER_COLOR "#fb8512"

struct winconf {
	int16_t		x, y;
	uint16_t	width, height;
	uint8_t		stackmode;
};

void window_show(xcb_window_t win);
void window_raise(xcb_window_t win);
void window_center_pointer(xcb_window_t win, int16_t width, int16_t height);
void window_set_focus(xcb_window_t win);
void window_move(xcb_window_t win, const uint16_t x, const uint16_t y);
void window_resize(xcb_window_t win, const uint16_t width, const uint16_t height);
void window_move_resize(xcb_window_t win, const uint16_t x, const uint16_t y, const uint16_t width, const uint16_t height);
bool window_get_geom(xcb_window_t win, int16_t *x, int16_t *y, uint16_t *width, uint16_t *height);
bool window_check_type(xcb_window_t win);
void window_setup(xcb_window_t win);
void window_get_limits_size(xcb_window_t win, uint16_t *max_width, uint16_t *max_height, uint16_t *min_width, uint16_t *min_height);
bool window_hint_us_position(xcb_window_t win);
bool window_get_pointer(const xcb_window_t *win, int16_t *x, int16_t *y);
void window_config(xcb_window_t win, uint16_t mask, const struct winconf *wc);
void window_delete(xcb_window_t win);
void window_unmap(xcb_window_t win);
void window_toggle_borders(xcb_window_t win, bool enable);

#endif
