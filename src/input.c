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
#define CLEANMASK(mask) (mask & ~(numlockmask | XCB_MOD_MASK_LOCK))

#include "config.h"

/* Mouse buttons */
typedef struct {
	unsigned int	mask, button;
	void (*func)(const Arg *);
	const Arg	arg;
	const bool	root_only;
} Button;

static Button buttons[] = {
	{ MOD, XCB_BUTTON_INDEX_1, mousemotion, { .i = WIN_MOVE   } },
	{ MOD, XCB_BUTTON_INDEX_3, mousemotion, { .i = WIN_RESIZE } },
};
unsigned int numlockmask;

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

/* the wm should listen to key presses */
void grabkeys(void)
{
	xcb_keycode_t *keycode;
	int i, k, m;
	unsigned int modifiers[] = {
		0,
		XCB_MOD_MASK_LOCK,
		numlockmask,
		numlockmask | XCB_MOD_MASK_LOCK
	};

	xcb_ungrab_key(conn, XCB_GRAB_ANY, screen->root, XCB_MOD_MASK_ANY);

	for (i = 0; i < LENGTH(keys); i++) {
		keycode = xcb_get_keycodes(keys[i].keysym);

		for (k = 0; keycode[k] != XCB_NO_SYMBOL; k++)
			for (m = 0; m < LENGTH(modifiers); m++)
				xcb_grab_key(conn, 1, screen->root, keys[i].mod
					     | modifiers[m], keycode[k],
					     XCB_GRAB_MODE_ASYNC,       /*pointer mode */
					     XCB_GRAB_MODE_ASYNC        /*keyboard mode */
					     );
		free(keycode);                                          /* allocated in xcb_get_keycodes() */
	}
}

bool setup_keyboard(void)
{
	xcb_get_modifier_mapping_reply_t *reply;
	xcb_keycode_t *modmap, *numlock;
	unsigned int i, j, n;

	reply = xcb_get_modifier_mapping_reply(conn,
					       xcb_get_modifier_mapping_unchecked(conn),
					       NULL);

	if (!reply)
		return false;

	modmap = xcb_get_modifier_mapping_keycodes(reply);

	if (!modmap)
		return false;

	numlock = xcb_get_keycodes(XK_Num_Lock);

	for (i = 4; i < 8; i++) {
		for (j = 0; j < reply->keycodes_per_modifier; j++) {
			xcb_keycode_t keycode = modmap[i * reply->keycodes_per_modifier + j];

			if (keycode == XCB_NO_SYMBOL)
				continue;

			if (numlock != NULL) {
				for (n = 0; numlock[n] != XCB_NO_SYMBOL; n++)
					if (numlock[n] == keycode) {
						numlockmask = 1 << i;
						break;
					}
			}
		}
	}

	free(reply);
	free(numlock);

	return true;
}

bool input_init(void)
{
	grabkeys();

	if (!setup_keyboard())
		return false;
	return true;
}

/* set the given client to listen to button events (presses / releases) */
void input_grab_buttons(struct client *c)
{
	unsigned int modifiers[] = {
		0,
		XCB_MOD_MASK_LOCK,
		numlockmask,
		numlockmask | XCB_MOD_MASK_LOCK
	};
	unsigned int b, m;

	for (b = 0; b < LENGTH(buttons); b++)
		if (!buttons[b].root_only) {
			for (m = 0; m < LENGTH(modifiers); m++)
				xcb_grab_button(conn, 1, c->id,
						XCB_EVENT_MASK_BUTTON_PRESS,
						XCB_GRAB_MODE_ASYNC,
						XCB_GRAB_MODE_ASYNC,
						screen->root, XCB_NONE,
						buttons[b].button,
						buttons[b].mask | modifiers[m]
						);
		}
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

void input_key_handler(xcb_key_press_event_t *ev)
{
	unsigned int i;
	xcb_keysym_t keysym = xcb_get_keysym(ev->detail);

	for (i = 0; i < LENGTH(keys); i++) {
		if (keysym == keys[i].keysym && CLEANMASK(keys[i].mod)
		    == CLEANMASK(ev->state) && keys[i].func) {
			keys[i].func(&keys[i].arg);
			break;
		}
	}
}

void input_button_handler(xcb_button_press_event_t *ev)
{
	unsigned int i;

	for (i = 0; i < LENGTH(buttons); i++)
		if (buttons[i].func && buttons[i].button == ev->detail
		    && CLEANMASK(buttons[i].mask)
		    == CLEANMASK(ev->state)) {
			if ((focuswin == NULL) && buttons[i].func == mousemotion)
				return;
			if (buttons[i].root_only) {
				if (ev->event == ev->root && ev->child == 0)
					buttons[i].func(&(buttons[i].arg));
			} else {
				buttons[i].func(&(buttons[i].arg));
 			}
 		}
}
