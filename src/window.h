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

#ifndef WINDOW_H
#define WINDOW_H

void get_borders_all_mons(uint16_t *border_x, uint16_t *border_y);
void getmonsize(int16_t *mon_x, int16_t *mon_y, uint16_t *mon_width,
		uint16_t *mon_height, const struct client *client);
void window_raise_current(void);
void window_raise(xcb_drawable_t win);
void window_center_pointer(xcb_drawable_t win, struct client *cl);
void window_set_unfocus(void);
void window_set_focus(struct client *client);
void window_moveresize(xcb_drawable_t win, const uint16_t x, const uint16_t y,
		       const uint16_t width, const uint16_t height);
void window_resize(xcb_drawable_t win, const uint16_t width, const uint16_t height);
void window_resize_limit(struct client *client);
void window_fitonscreen(struct client *client);
void window_arrange_all(void);
void window_move(xcb_drawable_t win, const int16_t x, const int16_t y);
void window_move_limit(struct client *client);
void window_unmax(struct client *client);
void window_max(struct client *client, uint16_t mon_x, uint16_t mon_y,
		uint16_t mon_width, uint16_t mon_height);
void window_maprequest(xcb_map_request_event_t *ev);
void window_configurerequest(xcb_configure_request_event_t *ev);

#endif
