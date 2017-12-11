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

#define BUTTONMASK      XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE

/* Change focus to next in window ring. */
void focusnext_helper(bool arg)
{
	struct client *cl = NULL;

	/* If we currently have no focus focus first in list. */
	if (NULL == focuswin || NULL == focuswin->win) {
		cl = winlist->data;
		while (cl->iconic == true && cl->win->next != NULL)
			cl = cl->win->next->data;
	} else {
		if (arg == FOCUS_NEXT) {
			if (NULL == focuswin->win->prev) {
				/* We were at the head of list.
				 * Focusing on last window in list unless
				 * we were already there.*/
				cl = winlist->data;

				/* Go to the end of the list */
				while (cl->win->next != NULL)
					cl = cl->win->next->data;
				/* walk backward until we find
				 * a window that isn't iconic */
				while (cl->iconic == true)
					cl = cl->win->prev->data;
			} else
			if (focuswin != winlist->data) {
				cl = focuswin->win->prev->data;
				while (cl->iconic == true
				       && cl->win->prev
				       != NULL)
					cl = cl->win->prev->data;
				/* move to the head an didn't find a
				 * window to focus so move to the end
				 * starting from the focused win */
				if (cl->iconic == true) {
					cl = focuswin;
					/* Go to the end of the list */
					while (cl->win->next
					       != NULL)
						cl = cl->win->next->data;
					while (cl->iconic == true)
						cl = cl->win->prev->data;
				}
			}
		} else {
			/* We were at the tail of list.
			 * Focusing on last window in list unless we
			 * were already there.*/
			if (NULL == focuswin->win->next) {
				/* We were at the end of list.
				 * Focusing on first window in list unless we
				 * were already there. */
				cl = winlist->data;
				while (cl->iconic && cl->win->next
				       != NULL)
					cl = cl->win->next->data;
			} else {
				cl = focuswin->win->next->data;
				while (cl->iconic == true
				       && cl->win->next
				       != NULL)
					cl = cl->win->next->data;
				/* we reached the end of the list without a
				 * new win to focus, so reloop from the head */
				if (cl->iconic == true) {
					cl = winlist->data;
					while (cl->iconic
					       && cl->win->next
					       != NULL)
						cl = cl->win->next->data;
				}
			}
		}
	}
	/* if NULL focuswin */
	if (NULL != cl && focuswin != cl && cl->iconic == false) {
		window_raise(cl->id);
		window_center_pointer(cl->id, cl);
		window_set_focus(cl);
	}
}

void focusnext(const Arg *arg)
{
	focusnext_helper(arg->i > 0);
}

void maxhalf(const Arg *arg)
{
	int16_t mon_x, mon_y;
	uint16_t mon_width, mon_height;

	if (NULL == focuswin || focuswin->maxed)
		return;

	getmonsize(&mon_x, &mon_y, &mon_width, &mon_height, focuswin);

	focuswin->y = mon_y;
	focuswin->height = mon_height;
	focuswin->width = ((float)(mon_width) / 2);

	if (arg->i == MAXHALF_VERTICAL_LEFT)
		focuswin->x = mon_x;
	else
		focuswin->x = mon_x + mon_width - focuswin->width;

	window_moveresize(focuswin->id, focuswin->x, focuswin->y,
		   focuswin->width, focuswin->height);

	focuswin->verthor = true;
	window_raise_current();
	window_fitonscreen(focuswin);
	window_center_pointer(focuswin->id, focuswin);
}

void changescreen(const Arg *arg)
{
	struct list *element;
	float xpercentage, ypercentage;

	if (NULL == focuswin || NULL == focuswin->monitor)
		return;

	if (arg->i == NEXT_SCREEN)
		element = focuswin->monitor->element->next;
	else
		element = focuswin->monitor->element->prev;

	if (NULL == element)
		return;

	xpercentage = (float)((focuswin->x - focuswin->monitor->x)
			      / (focuswin->monitor->width));
	ypercentage = (float)((focuswin->y - focuswin->monitor->y)
			      / (focuswin->monitor->height));

	focuswin->monitor = element->data;

	focuswin->x = focuswin->monitor->width * xpercentage
		+ focuswin->monitor->x + 0.5;
	focuswin->y = focuswin->monitor->height * ypercentage
		+ focuswin->monitor->y + 0.5;

	window_raise_current();
	window_fitonscreen(focuswin);
	window_move_limit(focuswin);
	window_center_pointer(focuswin->id, focuswin);
}

void deletewin(const Arg *arg)
{
	bool use_delete = false;
	xcb_icccm_get_wm_protocols_reply_t protocols;
	xcb_get_property_cookie_t cookie;
	uint32_t i;

	if (NULL == focuswin)
		return;

	/* Check if WM_DELETE is supported.  */
	cookie = xcb_icccm_get_wm_protocols_unchecked(conn, focuswin->id,
						      ewmh->WM_PROTOCOLS);

	if (xcb_icccm_get_wm_protocols_reply(conn, cookie, &protocols, NULL)
	    == 1) {
		for (i = 0; i < protocols.atoms_len; i++)
			if (protocols.atoms[i] == atom_get(wm_delete_window)) {
				xcb_client_message_event_t ev = {
					.response_type	= XCB_CLIENT_MESSAGE,
					.format		= 32,
					.sequence	= 0,
					.window		= focuswin->id,
					.type		= ewmh->WM_PROTOCOLS,
					.data.data32	= {
						atom_get(wm_delete_window),
						XCB_CURRENT_TIME
					}
				};

				xcb_send_event(conn, false, focuswin->id,
					       XCB_EVENT_MASK_NO_EVENT,
					       (char *)&ev
					       );

				use_delete = true;
				break;
			}
		xcb_icccm_get_wm_protocols_reply_wipe(&protocols);
	}
	if (!use_delete) xcb_kill_client(conn, focuswin->id);
}

static void unmax(struct client *client)
{
	if (NULL == client)
		return;

	client->x = client->origsize.x;
	client->y = client->origsize.y;
	client->width = client->origsize.width;
	client->height = client->origsize.height;

	client->maxed = client->hormaxed = 0;
	window_moveresize(client->id, client->x, client->y,
		   client->width, client->height);

	window_center_pointer(client->id, client);
}

static void saveorigsize(struct client *client)
{
	client->origsize.x = client->x;
	client->origsize.y = client->y;
	client->origsize.width = client->width;
	client->origsize.height = client->height;
}

static void maximize_helper(struct client *client,
			    uint16_t mon_x, uint16_t mon_y,
			    uint16_t mon_width, uint16_t mon_height)
{
	uint32_t values[4];

	values[0] = 0;
	saveorigsize(client);
	xcb_configure_window(conn, client->id, XCB_CONFIG_WINDOW_BORDER_WIDTH,
			     values);

	client->x = mon_x;
	client->y = mon_y;
	client->width = mon_width;
	client->height = mon_height;

	window_moveresize(client->id, client->x, client->y, client->width,
		   client->height);
	client->maxed = true;
}

void maximize(const Arg *arg)
{
	int16_t mon_x, mon_y;
	uint16_t mon_width, mon_height;

	if (NULL == focuswin)
		return;

	/* Check if maximized already. If so, revert to stored geometry. */
	if (focuswin->maxed) {
		unmax(focuswin);
		focuswin->maxed = false;
		return;
	}

	if (arg->i == FULLSCREEN_ONE_MONITOR) {
		getmonsize(&mon_x, &mon_y, &mon_width, &mon_height, focuswin);
	} else if (arg->i == FULLSCREEN_ALL_MONITOR) {
		mon_x = 0;
		mon_y = 0;
		monitor_borders(&mon_width, &mon_height);
	}

	maximize_helper(focuswin, mon_x, mon_y, mon_width, mon_height);
	window_raise_current();
	xcb_flush(conn);
}

void hide(const Arg *arg)
{
	if (focuswin == NULL)
		return;

	long data[] = {
		XCB_ICCCM_WM_STATE_ICONIC,
		ewmh->_NET_WM_STATE_HIDDEN,
		XCB_NONE
	};

	/* Unmap window and declare iconic. Unmapping will generate an
	 * UnmapNotify event so we can forget about the window later. */
	focuswin->iconic = true;

	xcb_unmap_window(conn, focuswin->id);
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, focuswin->id,
			    ewmh->_NET_WM_STATE, ewmh->_NET_WM_STATE, 32, 3, data);

	xcb_flush(conn);
}

void raise_all(const Arg *arg)
{
	struct client *cl = NULL;

	if (winlist && winlist->data)
		cl = winlist->data;
	else
		return;

	do {
		if (cl->iconic == true) {
			cl->iconic = false;
			xcb_map_window(conn, cl->id);
			xcb_flush(conn);
		}
	} while (cl->win->next && (cl = cl->win->next->data));
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

static xcb_cursor_t Create_Font_Cursor(xcb_connection_t *conn, uint16_t glyph)
{
	static xcb_font_t cursor_font;

	cursor_font = xcb_generate_id(conn);
	xcb_open_font(conn, cursor_font, strlen("cursor"), "cursor");
	xcb_cursor_t cursor = xcb_generate_id(conn);
	xcb_create_glyph_cursor(conn, cursor, cursor_font, cursor_font, glyph,
				glyph + 1, 0x3232, 0x3232, 0x3232, 0xeeee, 0xeeee, 0xeeec
				);

	return cursor;
}

/* Move window win as a result of pointer motion to coordinates rel_x,rel_y. */
void mousemove(const int16_t rel_x, const int16_t rel_y)
{
	if (focuswin == NULL)
		return;

	focuswin->x = rel_x;
	focuswin->y = rel_y;

	window_move_limit(focuswin);
}

void mouseresize(struct client *client, const int16_t rel_x, const int16_t rel_y)
{
	if (focuswin->id == screen->root || focuswin->maxed)
		return;

	client->width = abs(rel_x);
	client->height = abs(rel_y);

	window_resize_limit(client);
	client->vertmaxed = false;
	client->hormaxed = false;
}

void mousemotion(const Arg *arg)
{
	int16_t mx, my, winx, winy, winw, winh;
	xcb_query_pointer_reply_t *pointer;
	xcb_grab_pointer_reply_t *grab_reply;
	xcb_motion_notify_event_t *ev = NULL;
	xcb_generic_event_t *e = NULL;
	bool ungrab;

	pointer = xcb_query_pointer_reply(conn,
					  xcb_query_pointer(conn, screen->root), 0
					  );

	if (!pointer || focuswin->maxed) {
		free(pointer);
		return;
	}

	mx = pointer->root_x;
	my = pointer->root_y;
	winx = focuswin->x;
	winy = focuswin->y;
	winw = focuswin->width;
	winh = focuswin->height;

	xcb_cursor_t cursor;
	struct client example;
	window_raise_current();

	if (arg->i == WIN_MOVE) {
		cursor = Create_Font_Cursor(conn, 52);          /* fleur */
	} else {
		cursor = Create_Font_Cursor(conn, 120);         /* sizing */
		example = window_create_temp();
		xcb_map_window(conn, example.id);
	}

	grab_reply = xcb_grab_pointer_reply(conn, xcb_grab_pointer(conn, 0,
								   screen->root, BUTTONMASK | XCB_EVENT_MASK_BUTTON_MOTION
								   | XCB_EVENT_MASK_POINTER_MOTION, XCB_GRAB_MODE_ASYNC,
								   XCB_GRAB_MODE_ASYNC, XCB_NONE, cursor, XCB_CURRENT_TIME)
					    , NULL
					    );

	if (grab_reply->status != XCB_GRAB_STATUS_SUCCESS) {
		free(grab_reply);

		if (arg->i == WIN_RESIZE)
			xcb_unmap_window(conn, example.id);

		return;
	}

	free(grab_reply);
	ungrab = false;

	do {
		if (NULL != e)
			free(e);

		while (!(e = xcb_wait_for_event(conn)))
			xcb_flush(conn);

		switch (e->response_type & ~0x80) {
		case XCB_CONFIGURE_REQUEST:
		case XCB_MAP_REQUEST:
			events[e->response_type & ~0x80](e);
			break;
		case XCB_MOTION_NOTIFY:
			ev = (xcb_motion_notify_event_t *)e;
			if (arg->i == WIN_MOVE)
				mousemove(winx + ev->root_x - mx,
					  winy + ev->root_y - my);
			else
				mouseresize(&example, winw + ev->root_x - mx,
					    winh + ev->root_y - my);
			xcb_flush(conn);
			break;
		case XCB_KEY_PRESS:
		case XCB_KEY_RELEASE:
		case XCB_BUTTON_PRESS:
		case XCB_BUTTON_RELEASE:
			if (arg->i == WIN_RESIZE) {
				ev = (xcb_motion_notify_event_t *)e;
				mouseresize(focuswin, winw + ev->root_x - mx,
					    winh + ev->root_y - my);
			}
 
			ungrab = true;
			break;
		}
	} while (!ungrab && focuswin != NULL);
 
	free(pointer);
	free(e);
	xcb_free_cursor(conn, cursor);
	xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
 
	if (arg->i == WIN_RESIZE)
		xcb_unmap_window(conn, example.id);
 
	xcb_flush(conn);
}


void reload_conf(const Arg *arg)
{
	/* read conf */
	if (conf_read() == -1)
		LOGE("Fail to read conf");
	else {
		log_set_level(global_conf.log_level);
	}
}
