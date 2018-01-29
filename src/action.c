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

#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "global.h"
#include "action.h"
#include "atom.h"
#include "client.h"
#include "window.h"
#include "monitor.h"
#include "log.h"
#include "conf.h"
#include "cursor.h"

void change_focus(const Arg *arg)
{
	struct client *focus = client_get_focus();
	struct client *client = NULL;

	/* no focus, search first client */
	if (focus == NULL)
		client = client_get_first_from_head();
	else
		client = client_get_circular(focus, arg->i);

	/* found a client. show it */
	if (client != NULL && client != focus) {
		window_raise(client->id);
		window_center_pointer(client->id, client->width, client->height);
		window_set_focus(client->id);
	}
}

void max_half(const Arg *arg)
{
	int16_t mon_x, mon_y;
	uint16_t mon_width, mon_height;
	struct client *focus = client_get_focus();

	if (focus == NULL || focus->maxed)
		return;

	/* get monitor area of the focus */
	mon_x = focus->monitor->x;
	mon_y = focus->monitor->y;
	mon_width = focus->monitor->width;
	mon_height = focus->monitor->height;

	/* change height and width */
	focus->y = mon_y;
	focus->height = mon_height;
	focus->width = ((float)(mon_width) / 2);

	/* change position in X */
	if (arg->i == MAXHALF_VERTICAL_LEFT)
		focus->x = mon_x;
	else
		focus->x = mon_x + mon_width - focus->width;

	/* resize and show it */
	window_move_resize(focus->id, focus->x, focus->y, focus->width, focus->height);
	window_raise(focus->id);
	window_center_pointer(focus->id, focus->width, focus->height);
}

void delete_window(const Arg *arg)
{
	struct client *focus = client_get_focus();

	if (focus != NULL)
		window_delete(focus->id);
}

static void unmax(struct client *client)
{
	client->x = client->origsize.x;
	client->y = client->origsize.y;
	client->width = client->origsize.width;
	client->height = client->origsize.height;
	client->maxed = false;
}

static void max(struct client *client)
{
	client->origsize.x = client->x;
	client->origsize.y = client->y;
	client->origsize.width = client->width;
	client->origsize.height = client->height;
	client->maxed = true;
}

void maximize(const Arg *arg)
{
	struct client *focus = client_get_focus();

	if (focus == NULL)
		return;

	/* if maximized already, restore saved geometry. */
	if (focus->maxed)
		unmax(focus);
	else {
		/* change geometry */
		max(focus);
		if (arg->i == FULLSCREEN_ONE_MONITOR) {
			focus->x = focus->monitor->x;
			focus->y = focus->monitor->y;
			focus->width = focus->monitor->width;
			focus->height = focus->monitor->height;
		} else if (arg->i == FULLSCREEN_ALL_MONITOR) {
			focus->x = 0;
			focus->y = 0;
			monitor_borders(&focus->width, &focus->height);
		}
	}

	/* resize and show it */
	window_move_resize(focus->id, focus->x, focus->y, focus->width, focus->height);
	window_raise(focus->id);
	window_center_pointer(focus->id, focus->width, focus->height);
}

void hide(const Arg *arg)
{
	struct client *focus = client_get_focus();

	if (focus == NULL)
		return;

	window_unmap(focus->id);
	focus->iconic = true;
}

static void raise_client(struct client *client)
{
	if (client->iconic == true) {
		client->iconic = false;
		window_show(client->id);
	}
}

void raise_all(const Arg *arg)
{
	client_foreach(raise_client);
}

void start(const Arg *arg)
{
	if (fork())
		return;

	if (conn)
		close(screen->root);

	setsid();
	execvp((char *)arg->com[0], (char **)arg->com);
}

void jwm_exit(const Arg *arg)
{
	exit(EXIT_SUCCESS);
}

static void mouse_move(struct client *focus, const int16_t rel_x, const int16_t rel_y)
{
	uint16_t border_x, border_y;
	monitor_borders(&border_x, &border_y);

	/* assign values of mouse motion */
	focus->x = rel_x;
	focus->y = rel_y;

	/* check if we are outside borders */
	if (focus->x < 0)
		focus->x = 0;
	if (focus->y < 0)
		focus->y = 0;
	if (focus->x + focus->width > border_x)
		focus->x = border_x - focus->width;
	if (focus->y + focus->height > border_y)
		focus->y = border_y - focus->height;

	client_check_monitor(focus);
	window_move(focus->id, focus->x, focus->y);
}

static void mouse_resize(struct client *focus, const int16_t rel_x, const int16_t rel_y)
{
	uint16_t border_x, border_y;
	monitor_borders(&border_x, &border_y);

	/* assign values of mouse motion */
	focus->width = abs(rel_x);
	focus->height = abs(rel_y);

	/* check if we are outside borders */
	if (focus->x + focus->width > border_x)
		focus->width = border_x - focus->x;
	if (focus->y + focus->height > border_y)
		focus->height = border_y - focus->y;

	client_check_monitor(focus);
	window_resize(focus->id, focus->width, focus->height);
}

void mouse_motion(const Arg *arg)
{
	int16_t winx, winy, winw, winh;
	struct client *focus = client_get_focus();

	/* check if we focus on window none max */
	if ((focus == NULL) || focus->maxed)
		return;

	/* raise focus window */
	window_raise(focus->id);

	/* set borders */
	window_toggle_borders(focus->id, true);

	/* focus coordinates */
	winx = focus->x;
	winy = focus->y;
	winw = focus->width;
	winh = focus->height;

	/* pointer outside the focus */
	int16_t mx, my;
	cursor_get_coordinates(&mx, &my);
	if (mx < winx || mx > winx + winw || my < winy || my > winy + winh)
		return;

	/* create/setup cursor */
	if (arg->i == WIN_MOVE)
		cursor_set(focus->id, MOVE);
	else
		cursor_set(focus->id, SIZING);

	/* root window grabs control of the pointer.
	   further pointer events are reported only to root window */
	cursor_grab();

	/* loop until release buttons */
	xcb_generic_event_t *ev = NULL;
	xcb_motion_notify_event_t *ev_motion = NULL;
	bool button_released = false;

	while (button_released == false) {

		xcb_flush(conn);

		if ((ev = xcb_wait_for_event(conn))) {

			switch (ev->response_type & ~0x80) {
			case XCB_MOTION_NOTIFY:
				ev_motion = (xcb_motion_notify_event_t *)ev;
				if (arg->i == WIN_MOVE)
					mouse_move(focus,
						   winx + ev_motion->root_x - mx,
						   winy + ev_motion->root_y - my);
				else
					mouse_resize(focus,
						     winw + ev_motion->root_x - mx,
						     winh + ev_motion->root_y - my);
				break;
			case XCB_BUTTON_RELEASE:
				button_released = true;
				break;
			}

			free(ev);
		}
	}

	/* releases the pointer */
	cursor_ungrab();

	/* disable borders */
	window_toggle_borders(focus->id, false);
}


void reload_conf(const Arg *arg)
{
	if (conf_read() == -1)
		LOGE("Fail to read conf");
	else {
		log_init();
		monitor_set_wallpaper();
	}
}
