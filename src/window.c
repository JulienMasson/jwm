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

#include <xcb/xcb_icccm.h>

#include "global.h"
#include "client.h"
#include "input.h"

/* default position of the cursor:
 * correct values are:
 * TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT, MIDDLE
 * All these are relative to the current window. */
enum { BOTTOM_RIGHT, BOTTOM_LEFT, TOP_RIGHT, TOP_LEFT, MIDDLE };
#define CURSOR_POSITION MIDDLE

/* Window configuration data. */
struct winconf {
	int16_t		x, y;
	uint16_t	width, height;
	uint8_t		stackmode;
	xcb_window_t	sibling;
};

void getmonsize(int16_t *mon_x, int16_t *mon_y, uint16_t *mon_width,
		uint16_t *mon_height, const struct client *client)
{
	if (client == NULL || client->monitor == NULL) {
		/* Window isn't attached to any monitor, so we use
		 * the root window size. */
		*mon_x = *mon_y = 0;
		*mon_width = screen->width_in_pixels;
		*mon_height = screen->height_in_pixels;
	} else {
		*mon_x = client->monitor->x;
		*mon_y = client->monitor->y;
		*mon_width = client->monitor->width;
		*mon_height = client->monitor->height;
	}
}

/* Raise window win to top of stack. */
void window_raise(xcb_drawable_t win)
{
	uint32_t values[] = { XCB_STACK_MODE_ABOVE };

	if (screen->root == win || 0 == win)
		return;

	xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_STACK_MODE, values);
	xcb_flush(conn);
}

void window_raise_current(void)
{
	window_raise(focuswin->id);
}

void window_center_pointer(xcb_drawable_t win, struct client *cl)
{
	int16_t cur_x, cur_y;

	cur_x = cur_y = 0;

	switch (CURSOR_POSITION) {
	case BOTTOM_RIGHT:
		cur_x += cl->width;
	case BOTTOM_LEFT:
		cur_y += cl->height; break;
	case TOP_RIGHT:
		cur_x += cl->width;
	case TOP_LEFT:
		break;
	default:
		cur_x = cl->width / 2;
		cur_y = cl->height / 2;
	}

	xcb_warp_pointer(conn, XCB_NONE, win, 0, 0, 0, 0, cur_x, cur_y);
}

/* Mark window win as unfocused. */
void window_set_unfocus(void)
{
/* xcb_set_input_focus(conn, XCB_NONE, XCB_INPUT_FOCUS_NONE,XCB_CURRENT_TIME); */
	if (NULL == focuswin || focuswin->id == screen->root)
		return;
}

/* Set focus on window client. */
void window_set_focus(struct client *client)
{
	long data[] = { XCB_ICCCM_WM_STATE_NORMAL, XCB_NONE };

	/* If client is NULL, we focus on whatever the pointer is on.
	 * This is a pathological case, but it will make the poor user able
	 * to focus on windows anyway, even though this windowmanager might
	 * be buggy. */
	if (client == NULL) {
		focuswin = NULL;
		xcb_set_input_focus(conn, XCB_NONE,
				    XCB_INPUT_FOCUS_POINTER_ROOT, XCB_CURRENT_TIME);
		xcb_window_t not_win = 0;

		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, screen->root,
				    ewmh->_NET_ACTIVE_WINDOW, XCB_ATOM_WINDOW, 32, 1,
				    &not_win);

		xcb_flush(conn);
		return;
	}

	/* Don't bother focusing on the root window or on the same window
	 * that already has focus. */
	if (client->id == screen->root)
		return;

	if (focuswin != NULL)
		window_set_unfocus(); /* Unset last focus. */

	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, client->id,
			    ewmh->_NET_WM_STATE, ewmh->_NET_WM_STATE, 32, 2, data);
	xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT, client->id,
			    XCB_CURRENT_TIME); /* Set new input focus. */
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, screen->root,
			    ewmh->_NET_ACTIVE_WINDOW, XCB_ATOM_WINDOW, 32, 1, &client->id);

	/* Remember the new window as the current focused window. */
	focuswin = client;

	input_grab_buttons(client->id);
}

void window_moveresize(xcb_drawable_t win, const uint16_t x, const uint16_t y,
		       const uint16_t width, const uint16_t height)
{
	uint32_t values[4] = { x, y, width, height };

	if (screen->root == win || 0 == win)
		return;

	xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_X
			     | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH
			     | XCB_CONFIG_WINDOW_HEIGHT, values);

	xcb_flush(conn);
}

/* Resize window win to width,height. */
void window_resize(xcb_drawable_t win, const uint16_t width, const uint16_t height)
{
	uint32_t values[2] = { width, height };

	if (screen->root == win || 0 == win)
		return;

	xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_WIDTH
			     | XCB_CONFIG_WINDOW_HEIGHT, values);

	xcb_flush(conn);
}

/* Resize with limit. */
void window_resize_limit(struct client *client)
{
	uint16_t border_x, border_y;

	monitor_borders(&border_x, &border_y);

	/* Minimum height */
	if (0 != client->min_height && client->height < client->min_height)
		client->height = client->min_height;

	/* Minimum width */
	if (0 != client->min_width && client->width < client->min_width)
		client->width = client->min_width;

	/* Maximum x */
	if (client->x + client->width > border_x)
		client->width = border_x - client->x;

	/* Maximum y */
	if (client->y + client->height > border_y)
		client->height = border_y - client->y;

	window_resize(client->id, client->width, client->height);
}

static void saveorigsize(struct client *client)
{
	client->origsize.x = client->x;
	client->origsize.y = client->y;
	client->origsize.width = client->width;
	client->origsize.height = client->height;
}

void window_max(struct client *client, uint16_t mon_x, uint16_t mon_y,
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

void window_move(xcb_drawable_t win, const int16_t x, const int16_t y)
{                                    /* Move window win to root coordinates x,y. */
	uint32_t values[2] = { x, y };

	if (screen->root == win || 0 == win)
		return;

	xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_X
			     | XCB_CONFIG_WINDOW_Y, values);

	xcb_flush(conn);
}

/* Fit client on physical screen, moving and resizing as necessary. */
void window_fitonscreen(struct client *client)
{
	int16_t mon_x, mon_y;
	uint16_t mon_width, mon_height;
	bool willmove, willresize;

	willmove = willresize = false;

	getmonsize(&mon_x, &mon_y, &mon_width, &mon_height, client);

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

/* Rearrange windows to fit new screen size. */
void window_arrange_all(void)
{
	struct client *client;
	struct list *element;

	/* Go through all windows and resize them appropriately to
	 * fit the screen. */

	for (element = winlist; element != NULL; element = element->next) {
		client = element->data;
		window_fitonscreen(client);
	}
}

/* Move the window to the corresponding screen */
void window_move_limit(struct client *client)
{
	int16_t mon_y, mon_x;
	int16_t middle_x;
	uint16_t border_x, border_y;

	/* select right monitor with the middle of the client window */
	middle_x = client->x + (client->width / 2);
	client->monitor = monitor_find_by_coord(middle_x, client->y);

	/* get borders with all monitors */
	monitor_borders(&border_x, &border_y);

	/* Is it outside the physical monitor or close to the side? */
	if (client->y < mon_y)
		client->y = mon_y;
	else if (client->y + client->height > border_y)
		client->y = border_y - client->height;

	if (client->x < mon_x)
		client->x = mon_x;
	else if (client->x + client->width > border_x)
		client->x = border_x - client->width;

	window_move(client->id, client->x, client->y);
}

void window_unmax(struct client *client)
{
	if (client == NULL)
		return;

	client->x = client->origsize.x;
	client->y = client->origsize.y;
	client->width = client->origsize.width;
	client->height = client->origsize.height;

	client->maxed = false;
	window_moveresize(client->id, client->x, client->y,
		   client->width, client->height);

	window_center_pointer(client->id, client);
}

static bool window_getgeom(const xcb_drawable_t *win, int16_t *x, int16_t *y,
			   uint16_t *width, uint16_t *height)
{
	xcb_get_geometry_reply_t *geom = xcb_get_geometry_reply(conn,
								xcb_get_geometry(conn, *win), NULL);

	if (geom == NULL)
		return false;

	*x = geom->x;
	*y = geom->y;
	*width = geom->width;
	*height = geom->height;

	free(geom);
	return true;
}

/* Set border colour, width and event mask for window. */
static struct client *window_setup(xcb_window_t win)
{
	unsigned int i;
	uint32_t values[2];
	xcb_atom_t a;
	xcb_size_hints_t hints;
	xcb_ewmh_get_atoms_reply_t win_type;
	struct list *element;
	struct client *client;

	if (xcb_ewmh_get_wm_window_type_reply(ewmh,
					      xcb_ewmh_get_wm_window_type(ewmh, win),
					      &win_type, NULL) == 1) {
		for (i = 0; i < win_type.atoms_len; i++) {
			a = win_type.atoms[i];
			if (a == ewmh->_NET_WM_WINDOW_TYPE_TOOLBAR || a
			    == ewmh->_NET_WM_WINDOW_TYPE_DOCK || a
			    == ewmh->_NET_WM_WINDOW_TYPE_DESKTOP) {
				xcb_ewmh_get_atoms_reply_wipe(&win_type);
				xcb_map_window(conn, win);
				return NULL;
			}
		}
	}

	values[0] = XCB_EVENT_MASK_ENTER_WINDOW;
	/* xcb_change_window_attributes(conn, win, XCB_CW_BACK_PIXEL, */
	/*              &conf.empty_col); */
	xcb_change_window_attributes_checked(conn, win, XCB_CW_EVENT_MASK,
					     values);

	/* Add this window to the X Save Set. */
	xcb_change_save_set(conn, XCB_SET_MODE_INSERT, win);

	/* Remember window and store a few things about it. */
	client = malloc(sizeof(struct client));
	if (client == NULL)
		return NULL;

	element = list_add(&winlist, client);
	if (element == NULL)
		return NULL;

	client->id = win;
	client->x = client->y = client->width = client->height
		= client->min_width = client->min_height = 0;

	client->max_width = screen->width_in_pixels;
	client->max_height = screen->height_in_pixels;
	client->usercoord = client->iconic = false;
	client->maxed = false;
	client->monitor = NULL;
	client->win = element;

	/* Get window geometry. */
	window_getgeom(&client->id, &client->x, &client->y, &client->width,
		&client->height);

	/* Get the window's incremental size step, if any.*/
	xcb_icccm_get_wm_normal_hints_reply(conn,
					    xcb_icccm_get_wm_normal_hints_unchecked(conn, win),
					    &hints, NULL
					    );

	/* The user specified the position coordinates.
	 * Remember that so we can use geometry later. */
	if (hints.flags & XCB_ICCCM_SIZE_HINT_US_POSITION)
		client->usercoord = true;

	if (hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE) {
		client->min_width = hints.min_width;
		client->min_height = hints.min_height;
	}

	if (hints.flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE) {
		client->max_width = hints.max_width;
		client->max_height = hints.max_height;
	}

	return client;
}

static bool window_get_pointer(const xcb_drawable_t *win, int16_t *x, int16_t *y)
{
	xcb_query_pointer_reply_t *pointer;

	pointer = xcb_query_pointer_reply(conn,
					  xcb_query_pointer(conn, *win), 0);

	*x = pointer->win_x;
	*y = pointer->win_y;

	free(pointer);

	if (pointer == NULL)
		return false;

	return true;
}

/* Set position, geometry and attributes of a new window and show it on
 * the screen.*/
void window_maprequest(xcb_map_request_event_t *ev)
{
	struct client *client;
	long data[] = {
		XCB_ICCCM_WM_STATE_NORMAL,
		XCB_NONE
	};

	/* The window is trying to map itself on the current workspace,
	 * but since it's unmapped it probably belongs on another workspace.*/
	if (client_find(&ev->window) != NULL)
		return;

	client = window_setup(ev->window);
	if (client == NULL)
		return;

	/* If we don't have specific coord map it where the pointer is.*/
	if (!client->usercoord) {
		if (!window_get_pointer(&screen->root, &client->x, &client->y))
			client->x = client->y = 0;

		client->x -= client->width / 2;
		client->y -= client->height / 2;
		window_move(client->id, client->x, client->y);
	}

	/* Find the physical output this window will be on */
	client->monitor = monitor_find_by_coord(client->x, client->y);

	/* Show window on screen. */
	window_fitonscreen(client);
	xcb_map_window(conn, client->id);
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, client->id,
			    ewmh->_NET_WM_STATE, ewmh->_NET_WM_STATE, 32, 2, data);

	window_center_pointer(ev->window, client);
	client_update_list();
}

/* Helper function to configure a window. */
static void window_config(xcb_window_t win, uint16_t mask, const struct winconf *wc)
{
	uint32_t values[7];
	int8_t i = -1;

	if (mask & XCB_CONFIG_WINDOW_X) {
		mask |= XCB_CONFIG_WINDOW_X;
		i++;
		values[i] = wc->x;
	}

	if (mask & XCB_CONFIG_WINDOW_Y) {
		mask |= XCB_CONFIG_WINDOW_Y;
		i++;
		values[i] = wc->y;
	}

	if (mask & XCB_CONFIG_WINDOW_WIDTH) {
		mask |= XCB_CONFIG_WINDOW_WIDTH;
		i++;
		values[i] = wc->width;
	}

	if (mask & XCB_CONFIG_WINDOW_HEIGHT) {
		mask |= XCB_CONFIG_WINDOW_HEIGHT;
		i++;
		values[i] = wc->height;
	}

	if (mask & XCB_CONFIG_WINDOW_SIBLING) {
		mask |= XCB_CONFIG_WINDOW_SIBLING;
		i++;
		values[i] = wc->sibling;
	}

	if (mask & XCB_CONFIG_WINDOW_STACK_MODE) {
		mask |= XCB_CONFIG_WINDOW_STACK_MODE;
		i++;
		values[i] = wc->stackmode;
	}

	if (i == -1)
		return;

	xcb_configure_window(conn, win, mask, values);
	xcb_flush(conn);
}

void window_configurerequest(xcb_configure_request_event_t *ev)
{
	struct client *client;
	struct winconf wc;
	uint32_t values[1];

	if ((client = client_find(&ev->window))) { /* Find the client. */

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

		/* XXX Do we really need to pass on sibling and stack mode
		 * configuration? Do we want to? */
		if (ev->value_mask & XCB_CONFIG_WINDOW_SIBLING) {
			values[0] = ev->sibling;
			xcb_configure_window(conn, ev->window,
					     XCB_CONFIG_WINDOW_SIBLING, values);
		}

		if (ev->value_mask & XCB_CONFIG_WINDOW_STACK_MODE) {
			values[0] = ev->stack_mode;
			xcb_configure_window(conn, ev->window,
					     XCB_CONFIG_WINDOW_STACK_MODE, values);
		}

		/* Check if window fits on screen after resizing. */
		if (!client->maxed) {
			window_resize_limit(client);
			window_move_limit(client);
		}
	} else {
		/* Unmapped window, pass all options except border width. */
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.sibling = ev->sibling;
		wc.stackmode = ev->stack_mode;

		window_config(ev->window, ev->value_mask, &wc);
	}
}
