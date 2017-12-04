/*
 * This file is part of the jwm distribution (https://github.com/JulienMasson/jwm).
 * Copyright (c) 2017 Julien Masson.
 *
 * jwm is derived from 2bwm (https://github.com/venam/2bwm)
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

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <xcb/randr.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_ewmh.h>
#include "list.h"
#include "definitions.h"
#include "types.h"
#include "event.h"
#include "config.h"
#include "monitor.h"

/* Internal Constants */
/* Globals */
unsigned int numlockmask;
int sigcode;                            /* Signal code. Non-zero if we've been interruped by a signal. */
xcb_connection_t *conn;                 /* Connection to X server. */
xcb_ewmh_connection_t *ewmh;            /* Ewmh Connection. */
xcb_screen_t *screen;                   /* Our current screen. */
struct list *winlist;                   /* Global list of all client windows. */
struct client *focuswin;                /* Current focus window. */

/* Global configuration.*/
static const char *atomnames[NB_ATOMS][1] = {
	{ "WM_DELETE_WINDOW" },
	{ "WM_CHANGE_STATE"  }
};
xcb_atom_t ATOM[NB_ATOMS];

/* Function bodies */
void cleanup(void)
{
	xcb_set_input_focus(conn, XCB_NONE, XCB_INPUT_FOCUS_POINTER_ROOT,
			    XCB_CURRENT_TIME);
	xcb_ewmh_connection_wipe(ewmh);
	xcb_flush(conn);

	if (ewmh != NULL)
		free(ewmh);

	xcb_disconnect(conn);
}

/* wrapper to get xcb keycodes from keysymbol */
xcb_keycode_t *
xcb_get_keycodes(xcb_keysym_t keysym)
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

/* Get a defined atom from the X server. */
static xcb_atom_t getatom(const char *atom_name)
{
	xcb_intern_atom_cookie_t atom_cookie = xcb_intern_atom(conn, 0,
							       strlen(atom_name), atom_name);

	xcb_intern_atom_reply_t *rep = xcb_intern_atom_reply(conn, atom_cookie,
							     NULL);

	/* XXX Note that we return 0 as an atom if anything goes wrong.
	 * Might become interesting.*/

	if (rep == NULL)
		return 0;

	xcb_atom_t atom = rep->atom;

	free(rep);
	return atom;
}

static void ewmh_init(void)
{
	if (!(ewmh = calloc(1, sizeof(xcb_ewmh_connection_t))))
		printf("Fail\n");

	xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(conn, ewmh);

	xcb_ewmh_init_atoms_replies(ewmh, cookie, (void *)0);
}

/* get screen of display */
xcb_screen_t *
xcb_screen_of_display(xcb_connection_t *con, int screen)
{
	xcb_screen_iterator_t iter;

	iter = xcb_setup_roots_iterator(xcb_get_setup(con));
	for (; iter.rem; --screen, xcb_screen_next(&iter))
		if (screen == 0)
			return iter.data;

	return NULL;
}

static bool setup(int scrno)
{
	unsigned int i;

	unsigned int values[1] = {
		XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
		| XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
		| XCB_EVENT_MASK_PROPERTY_CHANGE
		| XCB_EVENT_MASK_BUTTON_PRESS
	};

	screen = xcb_screen_of_display(conn, scrno);

	if (!screen)
		return false;

	ewmh_init();
	xcb_ewmh_set_wm_pid(ewmh, screen->root, getpid());
	xcb_ewmh_set_wm_name(ewmh, screen->root, 4, "jwm");

	xcb_atom_t net_atoms[] = {
		ewmh->_NET_SUPPORTED,		   ewmh->_NET_WM_DESKTOP,
		ewmh->_NET_NUMBER_OF_DESKTOPS,	   ewmh->_NET_CURRENT_DESKTOP,
		ewmh->_NET_ACTIVE_WINDOW,	   ewmh->_NET_WM_ICON,
		ewmh->_NET_WM_STATE,		   ewmh->_NET_WM_NAME,
		ewmh->_NET_SUPPORTING_WM_CHECK,	   ewmh->_NET_WM_STATE_HIDDEN,
		ewmh->_NET_WM_ICON_NAME,	   ewmh->_NET_WM_WINDOW_TYPE,
		ewmh->_NET_WM_WINDOW_TYPE_DOCK,	   ewmh->_NET_WM_WINDOW_TYPE_DESKTOP,
		ewmh->_NET_WM_WINDOW_TYPE_TOOLBAR, ewmh->_NET_WM_PID,
		ewmh->_NET_CLIENT_LIST,		   ewmh->_NET_CLIENT_LIST_STACKING,
		ewmh->WM_PROTOCOLS,		   ewmh->_NET_WM_STATE,
		ewmh->_NET_WM_STATE_DEMANDS_ATTENTION
	};

	xcb_ewmh_set_supported(ewmh, scrno, LENGTH(net_atoms), net_atoms);

	for (i = 0; i < NB_ATOMS; i++)
		ATOM[i] = getatom(atomnames[i][0]);

	/* init all monitors */
	monitor_init();

	if (!setup_keyboard())
		return false;

	xcb_generic_error_t *error =
		xcb_request_check(conn,
				  xcb_change_window_attributes_checked(conn,
								       screen->root,
								       XCB_CW_EVENT_MASK, values));
	xcb_flush(conn);

	if (error)
		return false;

	grabkeys();

	event_init();

	return true;
}

static void sigcatch(const int sig)
{
	sigcode = sig;
}

static void install_sig_handlers(void)
{
	struct sigaction sa;

	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_NOCLDSTOP;
	/*could not initialize signal handler */
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
		exit(-1);
	sa.sa_handler = sigcatch;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 1; /* Restart if interrupted by handler */
	if (sigaction(SIGINT, &sa, NULL) == -1
	    || sigaction(SIGHUP, &sa, NULL) == -1
	    || sigaction(SIGTERM, &sa, NULL) == -1)
		exit(-1);
}

int main(int argc, char **argv)
{
	int scrno;

	install_sig_handlers();
	atexit(cleanup);

	if (!xcb_connection_has_error(conn = xcb_connect(NULL, &scrno)))
		if (setup(scrno))
			event_loop();

	/* the WM has stopped running, because sigcode is not 0 */
	exit(sigcode);
}
