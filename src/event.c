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

#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/timerfd.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/randr.h>

#include "global.h"
#include "monitor.h"
#include "action.h"
#include "window.h"
#include "client.h"
#include "input.h"
#include "log.h"
#include "panel.h"

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
	client_map_request(ev);
}

static void configurerequest(xcb_generic_event_t *e)
{
	xcb_configure_request_event_t *ev = (xcb_configure_request_event_t *)e;
	client_configure_request(ev);
}

static void buttonpress(xcb_generic_event_t *e)
{
	xcb_button_press_event_t *ev = (xcb_button_press_event_t *)e;
	input_button_handler(ev);
	panel_click(ev);
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
	panel_remove_systray(ev);
}

static void clientmessage(xcb_generic_event_t *e)
{
	xcb_client_message_event_t *ev = (xcb_client_message_event_t *)e;
	client_message(ev);
	panel_add_systray(ev);
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

bool event_init(void)
{
	unsigned int values[1] = {
		XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
		| XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
		| XCB_EVENT_MASK_BUTTON_PRESS
		| XCB_EVENT_MASK_EXPOSURE
	};
	xcb_void_cookie_t cookie;
	xcb_generic_error_t *error;
	int i;

	/* Root screen listen to "values" events */
	cookie = xcb_change_window_attributes_checked(conn,
						      screen->root,
						      XCB_CW_EVENT_MASK,
						      values);
	error = xcb_request_check(conn, cookie);
	if (error) {
		LOGE("Change window attributes failed: %d", error->error_code);
		return false;
	}

	/* set events */
	for (i = 0; i < XCB_NO_OPERATION; i++)
		events[i] = NULL;

	/* XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT */
	events[XCB_MAP_REQUEST] = maprequest;
	events[XCB_CONFIGURE_REQUEST] = configurerequest;
	events[XCB_CLIENT_MESSAGE] = clientmessage;

	/* XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY */
	events[XCB_DESTROY_NOTIFY] = destroynotify;
	events[XCB_ENTER_NOTIFY] = enternotify;
	events[XCB_UNMAP_NOTIFY] = unmapnotify;

	/* XCB_EVENT_MASK_BUTTON_PRESS */
	events[XCB_BUTTON_PRESS] = buttonpress;
	events[XCB_KEY_PRESS] = keypress;

	install_sig_handlers();
	return true;
}

void event_loop(void)
{
	int fd;
	xcb_generic_event_t *ev;

	/* panel refresh */
	struct panel *panel = panel_get();
	int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
	struct itimerspec itimer;
	itimer.it_value.tv_sec = panel->refresh;
	itimer.it_value.tv_nsec = 0;
	itimer.it_interval.tv_sec = panel->refresh;
	itimer.it_interval.tv_nsec = 0;
	if (timerfd_settime(timer_fd, 0, &itimer, NULL))
		LOGE("Failed to start timer");

	/* X events */
	int xfd = xcb_get_file_descriptor(conn);

	/* select vars */
	int rc;
	fd_set fds;
	int max_fd = timer_fd > xfd ? timer_fd : xfd;

	sigcode = 0;

	while (sigcode == 0) {

		/* init file descriptors */
		FD_ZERO(&fds);
		FD_SET(xfd, &fds);
		FD_SET(timer_fd, &fds);

		/* waiting until file descriptors become "ready" */
		rc = select(max_fd + 1, &fds, NULL, NULL, NULL);
		if (rc < 0) {
			if (errno == EINTR)
				break;
			LOGE("select()");
			break;
		}

		/* check which fd is available to read */
		for (fd = 0; fd <= max_fd; fd++) {

			if (FD_ISSET(fd, &fds)) {

				/* X events */
				if (fd == xfd) {
					if (xcb_connection_has_error(conn))
						abort();

					while ((ev = xcb_poll_for_event(conn))) {

						/* expose event only for panel */
						if ((ev->response_type & ~0x80) == XCB_EXPOSE)
							panel_event((xcb_expose_event_t *)ev);

						/* monitor event */
						monitor_event(ev->response_type);

						if (events[ev->response_type & ~0x80])
							events[ev->response_type & ~0x80](ev);

						free(ev);
					}

					xcb_flush(conn);

				} else if (fd == timer_fd) {
					/* panel refresh */
					panel_draw();
					timerfd_settime(timer_fd, 0, &itimer, NULL);
				}
			}
		}
	}
}

void event_exit(void)
{
	/* the WM has stopped running, because sigcode is not 0 */
	exit(sigcode);
}
