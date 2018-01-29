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

#include "global.h"
#include "list.h"
#include "client.h"
#include "window.h"
#include "action.h"
#include "atom.h"
#include "input.h"
#include "panel.h"

/* list of all client windows */
struct list *clients_head;

/* current focus client */
struct client *focus;

static struct client *client_find_by_win(xcb_window_t *win)
{
	struct client *client;
	struct list *index;

	for (index = clients_head; index != NULL; index = index->next) {
		client = index->data;

		if (*win == client->id)
			return client;
	}

	return NULL;
}

static struct client *client_create(xcb_window_t win)
{
	struct list *index;
	struct client *client;

	client = malloc(sizeof(struct client));
	if (client == NULL)
		return NULL;

	index = list_add(&clients_head, client);
	if (index == NULL)
		return NULL;

	client->id = win;
	client->x = client->y = client->width = client->height
		= client->min_width = client->min_height = 0;

	client->origsize.x = client->origsize.y =
		client->origsize.width = client->origsize.height = 0;

	client->max_width = screen->width_in_pixels;
	client->max_height = screen->height_in_pixels;
	client->iconic = false;
	client->maxed = false;
	client->monitor = NULL;
	client->index = index;

	return client;
}

static void client_remove(struct client *client)
{
	if (client == NULL)
		return;

	/* remove from clients_head list */
	list_remove(&clients_head, client->index);
}

void client_foreach(void (*func)(struct client *client))
{
	struct client *client;
	struct list *index;

	if (func == NULL)
		return;

	for (index = clients_head; index != NULL; index = index->next) {
		client = index->data;
		func(client);
	}
}

struct client *client_get_focus(void)
{
	return focus;
}

struct client *client_get_first_from_head(void)
{
	struct client *client = NULL;

	if (clients_head != NULL && clients_head->data != NULL) {
		client = clients_head->data;
		while (client->iconic == true && client->index->next != NULL)
				client = client->index->next->data;
	}

	return client;
}

struct client *client_get_circular(struct client *start, enum client_search_t direction)
{
	struct client *client = NULL;

	/* check start client */
	if (start == NULL || start->index == NULL)
		return client;

	/* search next client on the list */
	if (direction == CLIENT_NEXT) {

		/* tail of the list, get the first from the head */
		if (start->index->next == NULL)
			client = client_get_first_from_head();
		else {
			client = start->index->next->data;
			while (client->iconic == true && client->index->next != NULL)
				client = client->index->next->data;
		}
	} else {

		/* search client not iconic from the head to the start */
		if (start->index->prev) {
			client = start->index->prev->data;
			while (client->iconic == true && client->index->prev != NULL)
				client = client->index->prev->data;
		}
		
		/* already on the head with no client found, go to the end 
		   and walk backward until we find a client that isn't iconic */
		if (client == NULL && start->index->prev == NULL) {
			if (start->index->next) {
				client = start->index->next->data;
				while (client->index->next != NULL)
					client = client->index->next->data;
				while (client->iconic == true)
					client = client->index->prev->data;
			}
		}
	}

	return client;
}

void client_check_monitor(struct client *client)
{
	struct monitor *current_mon;

	/* check if we change monitor */
	current_mon = monitor_find_by_coord(client->x, client->y);
	if (client->monitor != current_mon)
		client->monitor = current_mon;
}

void client_fit_on_screen(struct client *client)
{
	int16_t mon_x, mon_y;
	uint16_t mon_width, mon_height;
	bool willmove, willresize;
	struct panel *panel = panel_get();

	willmove = willresize = false;

	if (client == NULL || client->monitor == NULL)
		return;

	/* get monitor area */
	mon_x = client->monitor->x;
	mon_y = client->monitor->y;
	mon_width = client->monitor->width;
	mon_height = client->monitor->height;

	/* change area if panel enable */
	if (panel->enable == true) {
		mon_y = mon_y + panel->height;
		mon_height = mon_height - panel->height;
	}

	/* outside the physical monitor */
	if (client->x > mon_x + mon_width || client->y > mon_y + mon_height
	    || client->x < mon_x || client->y < mon_y) {
		willmove = true;
		if (client->x > mon_x + mon_width)
			client->x = mon_x + mon_width;

		if (client->y > mon_y + mon_height)
			client->y = mon_y + mon_height - client->height;

		if (client->x < mon_x)
			client->x = mon_x;
		if (client->y < mon_y)
			client->y = mon_y;
	}

	/* Minimum size of the client */
	if (client->min_height != 0 && client->height < client->min_height) {
		client->height = client->min_height;
		willresize = true;
	}

	if (client->min_width != 0 && client->width < client->min_width) {
		client->width = client->min_width;
		willresize = true;
	}

	/* If the window is larger than our screen, just place it in
	 * the corner and resize. */
	if (client->width > mon_width) {
		client->x = mon_x;
		client->width = mon_width;
		willmove = willresize = true;
	} else 	if (client->x + client->width > mon_x + mon_width) {
		client->x = mon_x + mon_width - client->width;
		willmove = true;
	}

	if (client->height > mon_height) {
		client->y = mon_y;
		client->height = mon_height;
		willmove = willresize = true;
	} else 	if (client->y + client->height > mon_y + mon_height) {
		client->y = mon_y + mon_height - client->height;
		willmove = true;
	}

	if (willmove)
		window_move(client->id, client->x, client->y);

	if (willresize)
		window_resize(client->id, client->width, client->height);
}


void client_monitor_updated(struct monitor *mon)
{
	struct client *client;
	struct list *index;

	for (index = clients_head; index != NULL; index = index->next) {
		client = index->data;

		if (client->monitor == mon)
			client_fit_on_screen(client);
	}
}

void client_monitor_reassign(struct monitor *old, struct monitor *new)
{
	struct client *client;
	struct list *index;

	for (index = clients_head; index != NULL; index = index->next) {
		client = index->data;

		if (client->monitor == old) {
			client->monitor = new;
			client_fit_on_screen(client);
		}
	}
}

static void client_set_focus(struct client *client)
{
	if (client != NULL) {
		window_set_focus(client->id);
		input_grab_buttons(client->id);
		focus = client;
	}
}

void client_map_request(xcb_map_request_event_t *ev)
{
	xcb_window_t *win = &ev->window;
	struct client *client;

	/* client already mapped */
	if (client_find_by_win(win) != NULL)
		return;

	/* don't add toolbar, dock or desktop type in client list */
	if (window_check_type(*win) == false)
		return;

	/* setup new window */
	window_setup(*win);

	/* new client */
	client = client_create(*win);
	if (client == NULL)
		return;

	/* get window geometry */
	window_get_geom(client->id, &client->x, &client->y, &client->width,
		&client->height);

	/* get limits of window size */
	window_get_limits_size(client->id, &client->max_width, &client->max_height,
			       &client->min_width, &client->min_height);

	/* if coord map not specified, use pointer coordinate */
	if (window_hint_us_position(client->id) == false) {
		if (window_get_pointer(&screen->root, &client->x, &client->y) == false)
			client->x = client->y = 0;

		client->x -= client->width / 2;
		client->y -= client->height / 2;
		window_move(client->id, client->x, client->y);
	}

	/* find the physical output this window will be on */
	client->monitor = monitor_find_by_coord(client->x, client->y);

	/* show client on screen */
	client_fit_on_screen(client);
	window_show(client->id);
	window_center_pointer(client->id, client->width, client->height);
}

void client_configure_request(xcb_configure_request_event_t *ev)
{
	struct client *client;
	struct winconf wc;

	/* find the client. */
	client = client_find_by_win(&ev->window);
	if (client) {
		if (!client->maxed) {
			if (ev->value_mask & XCB_CONFIG_WINDOW_WIDTH)
				client->width = ev->width;

			if (ev->value_mask & XCB_CONFIG_WINDOW_HEIGHT)
				client->height = ev->height;

			if (ev->value_mask & XCB_CONFIG_WINDOW_X)
				client->x = ev->x;

			if (ev->value_mask & XCB_CONFIG_WINDOW_Y)
				client->y = ev->y;
		}

		/* check if client fit on screen */
		if (!client->maxed)
			client_fit_on_screen(client);
	} else {
		/* unmapped window */
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.stackmode = ev->stack_mode;

		window_config(ev->window, ev->value_mask, &wc);
	}
}

void client_destroy(xcb_destroy_notify_event_t *ev)
{
	struct client *client = NULL;

	/* focus client set to NULL when destroyed */
	if (focus != NULL && focus->id == ev->window)
		focus = NULL;

	/* remove client found */
	client = client_find_by_win(&ev->window);
	if (client != NULL)
		client_remove(client);
}

void client_enter(xcb_enter_notify_event_t *ev)
{
	struct client *client = NULL;

	if (ev->mode == XCB_NOTIFY_MODE_NORMAL
	    || ev->mode == XCB_NOTIFY_MODE_UNGRAB) {

		/* already focus on this client */
		if (focus != NULL && ev->event == focus->id)
			return;

		/* focus on client found */
		client = client_find_by_win(&ev->event);
		if (client == NULL)
			return;
		client_set_focus(client);
	}
}

void client_unmap(xcb_unmap_notify_event_t *ev)
{
	struct client *client = NULL;

	client = client_find_by_win(&ev->window);
	if (client == NULL || client->index == NULL)
		return;

	if (focus != NULL && client->id == focus->id)
		focus = NULL;

	if (client->iconic == false)
		client_remove(client);
}

void client_message(xcb_client_message_event_t *ev)
{
	struct client *client = NULL;
	const Arg fake_arg;

	if ((ev->type == atom_get(wm_change_state) && ev->format == 32
	     && ev->data.data32[0] == XCB_ICCCM_WM_STATE_ICONIC)
	    || ev->type == ewmh->_NET_ACTIVE_WINDOW) {
		client = client_find_by_win(&ev->window);
		if (client == NULL)
			return;

		if (client->iconic == false) {
			if (ev->type == ewmh->_NET_ACTIVE_WINDOW)
				client_set_focus(client);
			else
				hide(&fake_arg);
			return;
		}

		client->iconic = false;
		window_show(client->id);
		client_set_focus(client);
	}
}
