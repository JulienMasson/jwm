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
#include "window.h"
#include "atom.h"

xcb_window_t window_create(uint16_t x, uint16_t y, uint16_t width,
			   uint16_t height)
{
	xcb_window_t id = xcb_generate_id(conn);
	uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT
			| XCB_CW_EVENT_MASK;
	uint32_t values[3] = {screen->black_pixel, 1,
			      XCB_EVENT_MASK_EXPOSURE
				      | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY};

	xcb_create_window(conn,
			  /* depth */
			  screen->root_depth,
			  /* window id */
			  id,
			  /* parent window */
			  screen->root,
			  /* coordinates */
			  x, y, width, height,
			  /* border size */
			  0,
			  /* class */
			  XCB_WINDOW_CLASS_INPUT_OUTPUT,
			  /* visual */
			  screen->root_visual, mask, values);

	return id;
}

void window_show(xcb_window_t win)
{
	xcb_map_window(conn, win);
	xcb_flush(conn);
}

void window_raise(xcb_window_t win)
{
	uint32_t values[] = {XCB_STACK_MODE_ABOVE};

	if (win == screen->root)
		return;

	xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_STACK_MODE, values);
	xcb_flush(conn);
}

void window_center_pointer(xcb_window_t win, int16_t width, int16_t height)
{
	int16_t cur_x, cur_y;

	cur_x = width / 2;
	cur_y = height / 2;

	xcb_warp_pointer(conn, XCB_NONE, win, 0, 0, 0, 0, cur_x, cur_y);
	xcb_flush(conn);
}

void window_set_focus(xcb_window_t win)
{
	long data[] = {XCB_ICCCM_WM_STATE_NORMAL, XCB_NONE};

	if (win == screen->root)
		return;

	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win,
			    ewmh->_NET_WM_STATE, ewmh->_NET_WM_STATE, 32, 2,
			    data);
	xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT, win,
			    XCB_CURRENT_TIME);
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, screen->root,
			    ewmh->_NET_ACTIVE_WINDOW, XCB_ATOM_WINDOW, 32, 1,
			    &win);
	xcb_flush(conn);
}

void window_move(xcb_window_t win, const uint16_t x, const uint16_t y)
{
	uint32_t values[2] = {x, y};

	if (win == screen->root)
		return;

	xcb_configure_window(conn, win,
			     XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);
	xcb_flush(conn);
}

void window_resize(xcb_window_t win, const uint16_t width,
		   const uint16_t height)
{
	uint32_t values[2] = {width, height};

	if (win == screen->root)
		return;

	xcb_configure_window(conn, win,
			     XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
			     values);
	xcb_flush(conn);
}

void window_move_resize(xcb_window_t win, const uint16_t x, const uint16_t y,
			const uint16_t width, const uint16_t height)
{
	uint32_t values[4] = {x, y, width, height};

	if (win == screen->root)
		return;

	xcb_configure_window(conn, win,
			     XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y
				     | XCB_CONFIG_WINDOW_WIDTH
				     | XCB_CONFIG_WINDOW_HEIGHT,
			     values);
	xcb_flush(conn);
}

bool window_get_geom(xcb_window_t win, int16_t *x, int16_t *y, uint16_t *width,
		     uint16_t *height)
{
	xcb_get_geometry_cookie_t cookie = xcb_get_geometry(conn, win);
	xcb_get_geometry_reply_t *geom =
		xcb_get_geometry_reply(conn, cookie, NULL);

	if (geom == NULL)
		return false;

	*x = geom->x;
	*y = geom->y;
	*width = geom->width;
	*height = geom->height;

	free(geom);
	return true;
}

bool window_check_type(xcb_window_t win)
{
	unsigned int i;
	xcb_atom_t a;
	xcb_ewmh_get_atoms_reply_t win_type;

	/* detect if this window is toolbar, dock or desktop type */
	xcb_get_property_cookie_t cookie =
		xcb_ewmh_get_wm_window_type(ewmh, win);

	if (xcb_ewmh_get_wm_window_type_reply(ewmh, cookie, &win_type, NULL)
	    == 1) {
		for (i = 0; i < win_type.atoms_len; i++) {
			a = win_type.atoms[i];
			if (a == ewmh->_NET_WM_WINDOW_TYPE_TOOLBAR
			    || a == ewmh->_NET_WM_WINDOW_TYPE_DOCK
			    || a == ewmh->_NET_WM_WINDOW_TYPE_DESKTOP) {
				xcb_ewmh_get_atoms_reply_wipe(&win_type);
				xcb_map_window(conn, win);
				return false;
			}
		}
	}

	return true;
}

void window_setup(xcb_window_t win)
{
	uint32_t values[2];
	values[0] = XCB_EVENT_MASK_ENTER_WINDOW;
	xcb_change_window_attributes_checked(conn, win, XCB_CW_EVENT_MASK,
					     values);

	/* Add this window to the X Save Set. */
	xcb_change_save_set(conn, XCB_SET_MODE_INSERT, win);
}

void window_get_limits_size(xcb_window_t win, uint16_t *max_width,
			    uint16_t *max_height, uint16_t *min_width,
			    uint16_t *min_height)
{
	xcb_size_hints_t hints;
	xcb_get_property_cookie_t cookie =
		xcb_icccm_get_wm_normal_hints_unchecked(conn, win);

	xcb_icccm_get_wm_normal_hints_reply(conn, cookie, &hints, NULL);

	if (hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE) {
		*min_width = hints.min_width;
		*min_height = hints.min_height;
	}

	if (hints.flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE) {
		*max_width = hints.max_width;
		*max_height = hints.max_height;
	}
}

bool window_hint_us_position(xcb_window_t win)
{
	xcb_size_hints_t hints;
	xcb_get_property_cookie_t cookie =
		xcb_icccm_get_wm_normal_hints_unchecked(conn, win);

	xcb_icccm_get_wm_normal_hints_reply(conn, cookie, &hints, NULL);

	if (hints.flags & XCB_ICCCM_SIZE_HINT_US_POSITION)
		return true;
	return false;
}

bool window_get_pointer(const xcb_window_t *win, int16_t *x, int16_t *y)
{
	xcb_query_pointer_cookie_t cookie = xcb_query_pointer(conn, *win);
	xcb_query_pointer_reply_t *pointer =
		xcb_query_pointer_reply(conn, cookie, 0);

	if (pointer == NULL)
		return false;

	*x = pointer->win_x;
	*y = pointer->win_y;
	free(pointer);

	return true;
}

void window_config(xcb_configure_request_event_t *ev)
{
	uint16_t mask = ev->value_mask;
	uint32_t values[6];
	int8_t i = -1;

#pragma push_macro("CHECK_CONFIG")
#define CHECK_CONFIG(test, arg)                                                \
	if (mask & test) {                                                     \
		i++;                                                           \
		values[i] = ev->arg;                                           \
	}

	CHECK_CONFIG(XCB_CONFIG_WINDOW_X, x);
	CHECK_CONFIG(XCB_CONFIG_WINDOW_Y, y);
	CHECK_CONFIG(XCB_CONFIG_WINDOW_WIDTH, width);
	CHECK_CONFIG(XCB_CONFIG_WINDOW_HEIGHT, height);
	CHECK_CONFIG(XCB_CONFIG_WINDOW_STACK_MODE, stack_mode);
	CHECK_CONFIG(XCB_CONFIG_WINDOW_BORDER_WIDTH, border_width);
#pragma pop_macro("CHECK_CONFIG")

	if (i == -1)
		return;

	xcb_configure_window(conn, ev->window, mask, values);
	xcb_flush(conn);
}

void window_delete(xcb_window_t win)
{
	bool use_delete = false;
	xcb_icccm_get_wm_protocols_reply_t protocols;
	xcb_get_property_cookie_t cookie;
	uint32_t i;

	/* Check if WM_DELETE is supported.  */
	cookie = xcb_icccm_get_wm_protocols_unchecked(conn, win,
						      ewmh->WM_PROTOCOLS);

	if (xcb_icccm_get_wm_protocols_reply(conn, cookie, &protocols, NULL)
	    == 1) {
		for (i = 0; i < protocols.atoms_len; i++)
			if (protocols.atoms[i] == atom_get(wm_delete_window)) {
				xcb_client_message_event_t ev = {
					.response_type = XCB_CLIENT_MESSAGE,
					.format = 32,
					.sequence = 0,
					.window = win,
					.type = ewmh->WM_PROTOCOLS,
					.data.data32 = {
						atom_get(wm_delete_window),
						XCB_CURRENT_TIME}};

				xcb_send_event(conn, false, win,
					       XCB_EVENT_MASK_NO_EVENT,
					       (char *)&ev);
				use_delete = true;
				break;
			}
		xcb_icccm_get_wm_protocols_reply_wipe(&protocols);
	}
	if (!use_delete)
		xcb_kill_client(conn, win);
	xcb_flush(conn);
}

void window_unmap(xcb_window_t win)
{
	long data[] = {XCB_ICCCM_WM_STATE_ICONIC, ewmh->_NET_WM_STATE_HIDDEN,
		       XCB_NONE};

	xcb_unmap_window(conn, win);
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win,
			    ewmh->_NET_WM_STATE, ewmh->_NET_WM_STATE, 32, 3,
			    data);
	xcb_flush(conn);
}

static uint32_t window_get_color(const char *hex)
{
	uint32_t rgb48;
	char strgroups[7] = {hex[1], hex[2], hex[3], hex[4],
			     hex[5], hex[6], '\0'};

	rgb48 = strtol(strgroups, NULL, 16);
	return rgb48 | 0xff000000;
}

void window_toggle_borders(xcb_window_t win, bool enable)
{
	uint32_t values[1];

	if (enable == true) {
		values[0] = WINDOW_BORDER_WIDTH;
		xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_BORDER_WIDTH,
				     values);

		values[0] = window_get_color(WINDOW_BORDER_COLOR);
		xcb_change_window_attributes(conn, win, XCB_CW_BORDER_PIXEL,
					     values);
	} else {
		values[0] = 0;
		xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_BORDER_WIDTH,
				     values);
	}
	xcb_flush(conn);
}
