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

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>

#include "jwm.h"
#include "list.h"
#include "types.h"
#include "client.h"
#include "window.h"
#include "action.h"

static void addtoclientlist(const xcb_drawable_t id)
{
	xcb_change_property(conn, XCB_PROP_MODE_APPEND, screen->root, ewmh->_NET_CLIENT_LIST, XCB_ATOM_WINDOW, 32, 1, &id);
	xcb_change_property(conn, XCB_PROP_MODE_APPEND, screen->root, ewmh->_NET_CLIENT_LIST_STACKING, XCB_ATOM_WINDOW, 32, 1, &id);
}

/* Forget everything about client */
static void forgetclient(struct client *client)
{
	if (client == NULL)
		return;

	/* Remove from global window list */
	list_remove(&winlist, client->win);
}

/* Forget everything about a client with client->id win. */
static void forgetwin(xcb_window_t win)
{
	struct client *client;
	struct list *element;

	for (element = winlist; element != NULL; element = element->next) {
		/* Find this window in the global window list. */
		client = element->data;
		if (win == client->id) {
			/* Forget it and free allocated data, it might
			 * already be freed by handling an UnmapNotify. */
			forgetclient(client);
			return;
		}
	}
}

/* Find client with client->id win in global window list or NULL. */
struct client *client_find(const xcb_drawable_t *win)
{
	struct client *client;
	struct list *element;

	for (element = winlist; element != NULL; element = element->next) {
		client = element->data;

		if (*win == client->id)
			return client;
	}

	return NULL;
}

void client_update_list(void)
{
	uint32_t len, i;
	xcb_window_t *children;
	struct client *cl;

	/* can only be called after the first window has been spawn */
	xcb_query_tree_reply_t *reply = xcb_query_tree_reply(conn,
							     xcb_query_tree(conn, screen->root), 0);

	xcb_delete_property(conn, screen->root, ewmh->_NET_CLIENT_LIST);
	xcb_delete_property(conn, screen->root, ewmh->_NET_CLIENT_LIST_STACKING);

	if (reply == NULL) {
		addtoclientlist(0);
		return;
	}

	len = xcb_query_tree_children_length(reply);
	children = xcb_query_tree_children(reply);

	for (i = 0; i < len; i++) {
		cl = client_find(&children[i]);
		if (cl != NULL)
			addtoclientlist(cl->id);
	}

	free(reply);
}

void client_destroy(xcb_destroy_notify_event_t *ev)
{
	struct client *cl;

	if (NULL != focuswin && focuswin->id == ev->window)
		focuswin = NULL;

	cl = client_find(&ev->window);

	/* Find this window in list of clients and forget about it. */
	if (cl != NULL)
		forgetwin(cl->id);

	client_update_list();
}

void client_enter(xcb_enter_notify_event_t *ev)
{
	struct client *client;

	/*
	 * If this isn't a normal enter notify, don't bother. We also need
	 * ungrab events, since these will be generated on button and key
	 * grabs and if the user for some reason presses a button on the
	 * root and then moves the pointer to our window and releases the
	 * button, we get an Ungrab EnterNotify. The other cases means the
	 * pointer is grabbed and that either means someone is using it for
	 * menu selections or that we're moving or resizing. We don't want
	 * to change focus in those cases.
	 */

	if (ev->mode == XCB_NOTIFY_MODE_NORMAL
	    || ev->mode == XCB_NOTIFY_MODE_UNGRAB) {
		/* If we're entering the same window we focus now,
		 * then don't bother focusing. */

		if (NULL != focuswin && ev->event == focuswin->id)
			return;

		/* Otherwise, set focus to the window we just entered if we
		 * can find it among the windows we know about.
		 * If not, just keep focus in the old window. */

		client = client_find(&ev->event);
		if (client == NULL)
			return;

		window_set_focus(client);
	}
}

void client_unmap(xcb_unmap_notify_event_t *ev)
{
	struct client *client = NULL;

	/*
	 * Find the window in our current workspace list, then forget about it.
	 * Note that we might not know about the window we got the UnmapNotify
	 * event for.
	 * It might be a window we just unmapped on *another* workspace when
	 * changing workspaces, for instance, or it might be a window with
	 * override redirect set.
	 * This is not an error.
	 * XXX We might need to look in the global window list, after all.
	 * Consider if a window is unmapped on our last workspace while
	 * changing workspaces.
	 * If we do this, we need to keep track of our own windows and
	 * ignore UnmapNotify on them.
	 */

	client = client_find(&ev->window);
	if (NULL == client || client->win == NULL)
		return;

	if (focuswin != NULL && client->id == focuswin->id)
		focuswin = NULL;

	if (client->iconic == false)
		forgetclient(client);

	client_update_list();
}

void client_message(xcb_client_message_event_t *ev)
{
	struct client *cl;
	const Arg fake_arg;

	if ((ev->type == ATOM[wm_change_state] && ev->format == 32
	     && ev->data.data32[0] == XCB_ICCCM_WM_STATE_ICONIC)
	    || ev->type == ewmh->_NET_ACTIVE_WINDOW) {
		cl = client_find(&ev->window);

		if (cl == NULL)
			return;

		if (false == cl->iconic) {
			if (ev->type == ewmh->_NET_ACTIVE_WINDOW)
				window_set_focus(cl);
			else
				hide(&fake_arg);

			return;
		}

		cl->iconic = false;
		xcb_map_window(conn, cl->id);
		window_set_focus(cl);
	}
}
