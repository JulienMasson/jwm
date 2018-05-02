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

#include <stdbool.h>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>

#include "global.h"
#include "utils.h"
#include "action.h"
#include "input.h"
#include "client.h"

/* Super/Windows key */
#define MOD             XCB_MOD_MASK_4
#define CONTROL         XCB_MOD_MASK_CONTROL
#define SHIFT           XCB_MOD_MASK_SHIFT

#include "config.h"

/* Mouse buttons */
typedef struct {
	unsigned int	mask;
	unsigned int	button;
	void (*func)(const Arg *);
	const Arg	arg;
} Button;

static Button buttons[] = {
	{ MOD, XCB_BUTTON_INDEX_1, mouse_motion, { .i = WIN_MOVE   } },
	{ MOD, XCB_BUTTON_INDEX_3, mouse_motion, { .i = WIN_RESIZE } },
};

/* wrapper to get xcb keycodes from keysymbol */
static xcb_keycode_t *xcb_get_keycodes(xcb_keysym_t keysym)
{
	xcb_key_symbols_t *keysyms;
	xcb_keycode_t *keycode;

	if (!(keysyms = xcb_key_symbols_alloc(conn)))
		return NULL;

	keycode = xcb_key_symbols_get_keycode(keysyms, keysym);
	xcb_key_symbols_free(keysyms);

	return keycode;
}

/* wrapper to get xcb keysymbol from keycode */
static xcb_keysym_t xcb_get_keysym(xcb_keycode_t keycode)
{
	xcb_key_symbols_t *keysyms;

	if (!(keysyms = xcb_key_symbols_alloc(conn)))
		return 0;

	xcb_keysym_t keysym = xcb_key_symbols_get_keysym(keysyms, keycode, 0);
	xcb_key_symbols_free(keysyms);

	return keysym;
}

bool input_init(void)
{
	xcb_keycode_t *keycode;
	uint8_t i, k;

	/* release any key combination on root windows */
	xcb_ungrab_key(conn, XCB_GRAB_ANY, screen->root, XCB_MOD_MASK_ANY);

	/* grab key on root windows */
	for (i = 0; i < LENGTH(keys); i++) {
		keycode = xcb_get_keycodes(keys[i].keysym);

		for (k = 0; keycode[k] != XCB_NO_SYMBOL; k++)
			xcb_grab_key(conn,
				     1,
				     screen->root,
				     keys[i].mod,
				     keycode[k],
				     XCB_GRAB_MODE_ASYNC,
				     XCB_GRAB_MODE_ASYNC);
		free(keycode);
	}

	return true;
}

/* setup the given windows to listen to button events (presses / releases) */
void input_grab_buttons(xcb_window_t grab_window)
{
	unsigned int b;

	for (b = 0; b < LENGTH(buttons); b++)
		xcb_grab_button(conn,
				1,
				grab_window,
				XCB_EVENT_MASK_BUTTON_PRESS,
				XCB_GRAB_MODE_ASYNC,
				XCB_GRAB_MODE_ASYNC,
				screen->root,
				XCB_NONE,
				buttons[b].button,
				buttons[b].mask);
}

void input_key_handler(xcb_key_press_event_t *ev)
{
	unsigned int i;
	xcb_keysym_t keysym = xcb_get_keysym(ev->detail);

	for (i = 0; i < LENGTH(keys); i++) {
		if (keys[i].func &&
		    keys[i].keysym == keysym &&
		    keys[i].mod == ev->state) {
			keys[i].func(&keys[i].arg);
			break;
		}
	}
}

void input_button_handler(xcb_button_press_event_t *ev)
{
	unsigned int i;

	for (i = 0; i < LENGTH(buttons); i++)
		if (buttons[i].func &&
		    buttons[i].button == ev->detail &&
		    buttons[i].mask == ev->state)
			buttons[i].func(&(buttons[i].arg));
}
