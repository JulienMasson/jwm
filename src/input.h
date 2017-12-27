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

#ifndef INPUT_H
#define INPUT_H

#include <X11/keysymdef.h>
#include <xcb/xcb_keysyms.h>

#include "action.h"

typedef struct {
	unsigned int	mod;
	xcb_keysym_t	keysym;
	void (*func)(const Arg *);
	const Arg	arg;
} key;

bool input_init(void);

void input_grab_buttons(xcb_window_t grab_window);

void input_key_handler(xcb_key_press_event_t *ev);

void input_button_handler(xcb_button_press_event_t *ev);

#endif
