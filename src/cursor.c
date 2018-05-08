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

#include "global.h"
#include "cursor.h"

#define BUTTON_MASK                                                            \
	(XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE           \
	 | XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_POINTER_MOTION)

/* NORMAL, MOVE, SIZING */
static uint16_t glyphs[LAST] = {68, 52, 120};
static xcb_cursor_t cursors_id[LAST] = {0, 0, 0};
static int current_cursor;

void cursor_init(void)
{
	int i;
	/* cursor font */
	xcb_font_t cursor_font;

	cursor_font = xcb_generate_id(conn);
	xcb_open_font(conn, cursor_font, strlen("cursor"), "cursor");

	/* create all cursors */
	for (i = 0; i < LAST; i++) {
		xcb_cursor_t cursor = xcb_generate_id(conn);

		xcb_create_glyph_cursor(conn, cursor, cursor_font, cursor_font,
					glyphs[i], glyphs[i] + 1, 0x3232,
					0x3232, 0x3232, 0xeeee, 0xeeee, 0xeeec);
		cursors_id[i] = cursor;
	}
	xcb_close_font(conn, cursor_font);

	/* default cursor to NORMAL for root windows */
	cursor_set(screen->root, NORMAL);
}

void cursor_set(xcb_window_t window, enum cursor_t cursor)
{
	uint32_t value_list = cursors_id[cursor];

	xcb_change_window_attributes(conn, window, XCB_CW_CURSOR, &value_list);
	current_cursor = cursor;
}

void cursor_get_coordinates(int16_t *mx, int16_t *my)
{
	xcb_query_pointer_cookie_t cookie_query;
	xcb_query_pointer_reply_t *pointer;

	cookie_query = xcb_query_pointer(conn, screen->root);
	pointer = xcb_query_pointer_reply(conn, cookie_query, 0);
	if (pointer) {
		*mx = pointer->root_x;
		*my = pointer->root_y;
		free(pointer);
	}
}

void cursor_grab(void)
{
	xcb_grab_pointer_reply_t *grab_reply;
	xcb_grab_pointer_cookie_t cookie_grab;

	cookie_grab = xcb_grab_pointer(conn, 0, screen->root, BUTTON_MASK,
				       XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
				       XCB_NONE, cursors_id[current_cursor],
				       XCB_CURRENT_TIME);
	grab_reply = xcb_grab_pointer_reply(conn, cookie_grab, NULL);
	if (grab_reply->status != XCB_GRAB_STATUS_SUCCESS) {
		free(grab_reply);
		return;
	}
	free(grab_reply);
}

void cursor_ungrab(void)
{
	xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
	xcb_flush(conn);
}
