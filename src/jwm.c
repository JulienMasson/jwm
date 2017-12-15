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

#include <stdbool.h>
#include <stdio.h>
#include <getopt.h>
#include "global.h"
#include "log.h"
#include "ewmh.h"
#include "monitor.h"
#include "input.h"
#include "atom.h"
#include "event.h"
#include "conf.h"

/* Globals */
xcb_connection_t *conn;                 /* Connection to X server. */
xcb_screen_t *screen;                   /* Our current screen. */
struct list *winlist;                   /* Global list of all client windows. */
struct client *focuswin;                /* Current focus window. */

/* Function bodies */
void cleanup(void)
{
	ewmh_exit();
	xcb_set_input_focus(conn, XCB_NONE, XCB_INPUT_FOCUS_POINTER_ROOT,
			    XCB_CURRENT_TIME);
	xcb_flush(conn);
	xcb_disconnect(conn);
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

static bool init(int scrno)
{
	unsigned int values[1] = {
		XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
		| XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
		| XCB_EVENT_MASK_PROPERTY_CHANGE
		| XCB_EVENT_MASK_BUTTON_PRESS
	};
	xcb_void_cookie_t cookie;
	xcb_generic_error_t *error;

	/* get screen */
	screen = xcb_screen_of_display(conn, scrno);
	if (!screen)
		return false;

	/* init screen */
	cookie = xcb_change_window_attributes_checked(conn,
						      screen->root,
						      XCB_CW_EVENT_MASK, values);
	error = xcb_request_check(conn, cookie);
	if (error)
		return false;

	/* ewmh init */
	ewmh_init(scrno);

	/* atom init */
	atom_init();

	/* init all monitors */
	monitor_init();

	/* init input, keyboard, button ... */
	if (!input_init())
		return false;

	/* init events */
	event_init();

	return true;
}

void usage(void)
{
	printf("NAME\n"
	       "       jwm - A minimalist floating WM, written over the XCB\n"
	       "\n"
	       "SYNOPSIS\n"
	       "       jwm [OPTION...]\n"
	       "\n"
	       "OPTIONS\n"
	       "       -c, --conf\n"
	       "              Specifies which configuration file to use instead of the default.\n"
	       "\n"
	       "       -h, --help\n"
	       "              Display this help and exits.\n"
	       "\n");
}

int main(int argc, char **argv)
{
	int screenp, ch;
	char *conf_file = NULL;
	struct option long_options[] = {
		{"conf", required_argument, NULL, 'c'},
		{"help", no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0}
	};

	/* parse args */
	while ((ch = getopt_long(argc, argv, "c:h:", long_options, NULL)) != -1) {
		switch (ch) {
		case 'c':
			conf_file = optarg;
			break;
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
			break;
		}
	}

	/* init log and load global conf */
	conf_init(conf_file);
	log_init();

	/* call cleanup on normal process termination */
	atexit(cleanup);

	/* init X connection and start main loop */
	LOGI("Start connection with X server");
	conn = xcb_connect(NULL, &screenp);
	if (xcb_connection_has_error(conn))
		LOGE("Connection has shut down due to a fatal error");
	else
		if (init(screenp)) {
			LOGI("Start main loop");
			event_loop();
		}

	/* exit from the main loop */
	LOGW("Exit main loop");
	event_exit();
}
