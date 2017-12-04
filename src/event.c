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

#include <stdlib.h>
#include <signal.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/randr.h>

#include "global.h"
#include "monitor.h"
#include "action.h"
#include "window.h"
#include "client.h"
#include "input.h"

void (*events[XCB_NO_OPERATION])(xcb_generic_event_t *e);

/* Signal code. Non-zero if we've been interruped by a signal. */
static int sigcode;

static void keypress(xcb_generic_event_t *e)
{
	xcb_key_press_event_t *ev = (xcb_key_press_event_t *)e;
	input_key_handler(ev);
}

static void maprequest(xcb_generic_event_t *e)
{
	xcb_map_request_event_t *ev = (xcb_map_request_event_t *)e;
	window_maprequest(ev);
}

static void configurerequest(xcb_generic_event_t *e)
{
	xcb_configure_request_event_t *ev = (xcb_configure_request_event_t *)e;
	window_configurerequest(ev);
}

static void buttonpress(xcb_generic_event_t *e)
{
	xcb_button_press_event_t *ev = (xcb_button_press_event_t *)e;
	input_button_handler(ev);
}

static void destroynotify(xcb_generic_event_t *e)
{
	xcb_destroy_notify_event_t *ev = (xcb_destroy_notify_event_t *)e;
	client_destroy(ev);
}

static void enternotify(xcb_generic_event_t *e)
{
	xcb_enter_notify_event_t *ev = (xcb_enter_notify_event_t *)e;
	client_enter(ev);
}

static void unmapnotify(xcb_generic_event_t *e)
{
	xcb_unmap_notify_event_t *ev = (xcb_unmap_notify_event_t *)e;
	client_unmap(ev);
}

static void clientmessage(xcb_generic_event_t *e)
{
	xcb_client_message_event_t *ev = (xcb_client_message_event_t *)e;
	client_message(ev);
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

void event_init(void)
{
	install_sig_handlers();

	/* set events */
	int i;
	for (i = 0; i < XCB_NO_OPERATION; i++)
		events[i] = NULL;

	events[XCB_KEY_PRESS] = keypress;
	events[XCB_MAP_REQUEST] = maprequest;
	events[XCB_CONFIGURE_REQUEST] = configurerequest;
	events[XCB_BUTTON_PRESS] = buttonpress;
	events[XCB_DESTROY_NOTIFY] = destroynotify;
	events[XCB_ENTER_NOTIFY] = enternotify;
	events[XCB_UNMAP_NOTIFY] = unmapnotify;
	events[XCB_CLIENT_MESSAGE] = clientmessage;
}

void event_loop(void)
{
	xcb_generic_event_t *ev;

	sigcode = 0;

	while (sigcode == 0) {

		/* the WM is running */
		xcb_flush(conn);

		if (xcb_connection_has_error(conn))
			abort();

		if ((ev = xcb_wait_for_event(conn))) {

			/* monitor event */
			monitor_event(ev->response_type);

			if (events[ev->response_type & ~0x80])
				events[ev->response_type & ~0x80](ev);

			free(ev);
		}
	}
}

void event_exit(void)
{
	/* the WM has stopped running, because sigcode is not 0 */
	exit(sigcode);
}
