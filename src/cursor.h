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

#ifndef CURSOR_H
#define CURSOR_H

#include <stdint.h>

enum cursor_t {
	NORMAL,
	MOVE,
	SIZING,
	LAST
};

void cursor_init(void);

void cursor_set(xcb_window_t window, enum cursor_t cursor);

void cursor_get_coordinates(int16_t *mx, int16_t *my);

void cursor_grab(void);

void cursor_ungrab(void);

#endif
