/*
 * This file is part of the jwm distribution:
 * https://github.com/JulienMasson/jwm
 *
 * Copyright (c) 2018 Julien Masson.
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

#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include "global.h"
#include "panel.h"
#include "window.h"
#include "monitor.h"
#include "client.h"
#include "utils.h"
#include "log.h"
#include "widgets.h"
#include "draw.h"

#define PANEL_FONT "sans 12"
#define PANEL_REFRESH 60
#define PANEL_TEXT_SIZE 11

struct panel_client_data {
	struct monitor *mon;
	double *pos;
	double *max_width;
};

struct panel_client {
	struct client *client;
	double pos;
	double width;
	struct list *index;
};

struct panel *panel = NULL;
struct list *panel_clients_head;
xcb_window_t *systray = NULL;
int systray_count = 0;

static struct panel_client *panel_client_add(struct client *client, double pos,
					     double width)
{
	struct list *index;
	struct panel_client *panel_client;

	panel_client = malloc(sizeof(struct panel_client));
	if (panel_client == NULL)
		return NULL;

	index = list_add(&panel_clients_head, panel_client);
	if (index == NULL)
		return NULL;

	panel_client->client = client;
	panel_client->pos = pos;
	panel_client->width = width;
	panel_client->index = index;

	return panel_client;
}

static struct panel_client *panel_client_find_by_client(struct client *client)
{
	struct panel_client *panel_client;
	struct list *index;

	for (index = panel_clients_head; index != NULL; index = index->next) {
		panel_client = index->data;

		if (client == panel_client->client)
			return panel_client;
	}

	return NULL;
}

static void panel_client_update(struct client *client, double pos, double width)
{
	struct panel_client *panel_client;
	struct list *index;

	for (index = panel_clients_head; index != NULL; index = index->next) {
		panel_client = index->data;

		if (client == panel_client->client) {
			panel_client->pos = pos;
			panel_client->width = width;
		}
	}
}

void panel_init(void)
{
	int16_t border_x, border_y;
	uint16_t border_width, border_height;

	monitor_borders(&border_x, &border_y, &border_width, &border_height);
	panel = malloc(sizeof(struct panel));

	/* init panel values */
	panel->id =
		window_create(border_x, border_y, border_width, PANEL_HEIGHT);
	panel->x = border_x;
	panel->y = border_y;
	panel->width = border_width;
	panel->height = PANEL_HEIGHT;
	panel->enable = true;
	panel->refresh = PANEL_REFRESH;

	/* init draw */
	panel->src = cairo_xcb_surface_create(conn, panel->id, visual,
					      panel->width - panel->x,
					      panel->height - panel->y);
	panel->draw = draw_create(panel->src, PANEL_FONT);

	/* init systray */
	char atomname[strlen("_NET_SYSTEM_TRAY_S") + 11];
	xcb_intern_atom_cookie_t stc;
	xcb_intern_atom_reply_t *bar_rpy;
	xcb_get_selection_owner_cookie_t gsoc;
	xcb_get_selection_owner_reply_t *gso_rpy;

	snprintf(atomname, strlen("_NET_SYSTEM_TRAY_S") + 11,
		 "_NET_SYSTEM_TRAY_S0");
	stc = xcb_intern_atom(conn, 0, strlen(atomname), atomname);
	if (!(bar_rpy = xcb_intern_atom_reply(conn, stc, NULL)))
		LOGE("could not get atom %s\n", atomname);
	xcb_set_selection_owner(conn, panel->id, bar_rpy->atom,
				XCB_CURRENT_TIME);

	gsoc = xcb_get_selection_owner(conn, bar_rpy->atom);
	if (!(gso_rpy = xcb_get_selection_owner_reply(conn, gsoc, NULL)))
		LOGE("could not get selection owner for %s\n", atomname);

	if (gso_rpy->owner != panel->id) {
		LOGE("Another system tray running?\n");
		free(gso_rpy);
	}

	/* Let everybody know that there is a system tray running */
	uint32_t buf[32];
	xcb_client_message_event_t *ev = (xcb_client_message_event_t *)buf;
	ev->response_type = XCB_CLIENT_MESSAGE;
	ev->window = screen->root;
	ev->type = XCB_ATOM_NONE;
	ev->format = 32;
	ev->data.data32[0] = XCB_CURRENT_TIME;
	ev->data.data32[1] = bar_rpy->atom;
	ev->data.data32[2] = panel->id;
	xcb_send_event(conn, 0, screen->root, 0xFFFFFF, (char *)buf);

	/* show panel */
	window_show(panel->id);

	/* init widgets window */
	widgets_init(panel->id, PANEL_HEIGHT);
}

struct panel *panel_get(void)
{
	return panel;
}

void panel_update_geom(void)
{
	int16_t border_x, border_y;
	uint16_t border_width, border_height;
	monitor_borders(&border_x, &border_y, &border_width, &border_height);

	if (panel
	    && ((border_x != panel->x) || (border_y != panel->y)
		|| (border_width != panel->width))) {
		/* set new values */
		panel->x = border_x;
		panel->y = border_y;
		panel->width = border_width;

		/* destroy draw */
		draw_destroy(panel->draw);
		cairo_surface_destroy(panel->src);

		/* init draw */
		panel->src = cairo_xcb_surface_create(conn, panel->id, visual,
						      panel->width - panel->x,
						      panel->height - panel->y);
		panel->draw = draw_create(panel->src, PANEL_FONT);

		/* move, resize and show panel */
		window_move_resize(panel->id, border_x, border_y, border_width,
				   PANEL_HEIGHT);
		panel_draw();
	}
}

static void draw_clock(double *max_width)
{
	time_t t;
	struct tm *lt;
	char buf_time[256];
	int width, height;

	/* get current date */
	t = time(NULL);
	lt = localtime(&t);
	buf_time[strftime(buf_time, sizeof(buf_time), "%R  -  %d %b", lt)] =
		'\0';

	/* Get width and height of the text */
	draw_get_text_prop(panel->draw, buf_time, strlen(buf_time), &width,
			   &height);

	/* update max width */
	width += 5;
	*max_width = panel->width - width;

	/* draw text */
	struct area_t area = {*max_width, (PANEL_HEIGHT - height) / 2, width,
			      panel->height};
	draw_set_color(panel->draw, GREY);
	draw_text(panel->draw, buf_time, strlen(buf_time), area);
	draw_set_color(panel->draw, BLACK);
}

static void draw_systray(double *max_width)
{
	int i;

	for (i = 0; i < systray_count; i++) {
		*max_width -= 24;
		window_move(systray[i], *max_width, 3);
	}
}

static void draw_widgets(double *max_width)
{
	xcb_window_t win = widgets_get_win();

	if (win != XCB_NONE) {
		*max_width -= widgets_get_width();
		window_move(win, *max_width, 0);
	}
}

static void get_icon_path(xcb_window_t win, char *icon_path)
{
	xcb_get_property_cookie_t cookie;
	char name[256];
	uint32_t pid;

	/* get pid from the window */
	memset(name, '\0', 256);
	cookie = xcb_ewmh_get_wm_pid(ewmh, win);
	if (xcb_ewmh_get_wm_pid_reply(ewmh, cookie, &pid, NULL))
		get_process_name(pid, name, 256);

	/* check access to icon path, fallback on the default */
	snprintf(icon_path, 256, "%s%s.png", ICONS_DIR, basename(name));
	if (file_access(icon_path) == false)
		snprintf(icon_path, 256, "%sdefault.png", ICONS_DIR);
}

static void get_window_name(xcb_window_t win, char *name, uint32_t len)
{
	xcb_get_property_cookie_t cookie;
	xcb_ewmh_get_utf8_strings_reply_t data;

	cookie = xcb_ewmh_get_wm_name(ewmh, win);
	if (xcb_ewmh_get_wm_name_reply(ewmh, cookie, &data, NULL)
	    && data.strings_len < len)
		strncpy(name, data.strings, data.strings_len);
}

static int panel_get_text_width(char *text, size_t len)
{
	int width = 0;
	draw_get_text_prop(panel->draw, text, len, &width, NULL);
	return width;
}

static void draw_client(struct client *client, void *data)
{
	struct client *focus = client_get_focus();
	struct panel_client_data *client_data =
		(struct panel_client_data *)data;
	char name[256];
	char icon_path[256];
	int width_name;

	/* check monitor */
	if (client->monitor != client_data->mon)
		return;

	/* get icon and name of the window */
	memset(name, '\0', 256);
	get_window_name(client->id, name, 256);
	get_icon_path(client->id, icon_path);

	/* if window name is too long, add "..." at the end */
	if (strlen(name) > 20) {
		name[17] = '.';
		name[18] = '.';
		name[19] = '.';
		name[20] = '\0';
	}

	/* draw rectangle, icon and name of the process */
	if (strlen(name) > 0) {

		width_name = panel_get_text_width(name, strlen(name));

		/* check if we can draw this client */
		if ((*client_data->pos + (width_name + 15 + 24)
		     > *client_data->max_width)
		    || (*client_data->pos + (width_name + 15 + 24)
			> client->monitor->width + client->monitor->x))
			return;

		/* add in panel clients list */
		if (panel_client_find_by_client(client) == NULL)
			panel_client_add(client, *client_data->pos,
					 width_name + 15 + 24);
		else
			panel_client_update(client, *client_data->pos,
					    width_name + 15 + 24);

		/* rounded rectangle around name */
		struct area_t rect_area = {*client_data->pos, 0,
					   width_name + 15 + 24, PANEL_HEIGHT};
		if ((client == focus) && (client->iconic == false))
			draw_set_color(panel->draw, ORANGE);
		else
			draw_set_color(panel->draw, GREY);
		draw_rounded_rectangle(panel->draw, rect_area);

		/* shift to display icon */
		*client_data->pos += 5;

		/* draw icon */
		struct area_t icon_area = {*client_data->pos, 3, 0, 0};
		draw_icon(panel->draw, icon_path, icon_area);

		/* shift to show name */
		*client_data->pos += 24 + 5;

		/* show client name */
		struct area_t client_area = {*client_data->pos, 4, width_name,
					     0};
		if ((client == focus) && (client->iconic == false))
			draw_set_color(panel->draw, ORANGE);
		else
			draw_set_color(panel->draw, GREY);
		draw_text(panel->draw, name, strlen(name), client_area);
		draw_set_color(panel->draw, BLACK);

		/* update position if next client */
		if (client->index->next != NULL)
			*client_data->pos += width_name + 5 + 4;
	}
}

static void draw_by_monitor(struct monitor *mon, void *data)
{
	struct panel_client_data *client_data =
		(struct panel_client_data *)data;
	client_data->mon = mon;
	*client_data->pos = mon->x + 1;

	/* reset panel_clients_head */
	struct list *index;
	for (index = panel_clients_head; index != NULL; index = index->next)
		list_remove(&panel_clients_head, index);

	client_foreach(draw_client, (void *)client_data);
}

void panel_draw(void)
{
	double pos = 0;
	double max_width = 0;
	struct panel_client_data client_data = {NULL, &pos, &max_width};

	if (panel->enable == true) {
		/* fill panel black */
		struct area_t area = {panel->x, panel->y, panel->width,
				      panel->height};
		draw_rectangle(panel->draw, area, true);

		/* draw on right of the panel:
		 *
		 * .... | widgets | systray | clock |
		 *
		 */
		draw_clock(client_data.max_width);
		draw_systray(client_data.max_width);
		draw_widgets(client_data.max_width);

		/* draw clients */
		monitor_foreach(draw_by_monitor, (void *)&client_data);

		/* flush */
		cairo_surface_flush(panel->src);
		xcb_flush(conn);
	}
}

void panel_event(xcb_expose_event_t *ev)
{
	/* only redraw on last expose event */
	if ((ev->count == 0) && (ev->window == panel->id))
		panel_draw();
}

static bool systray_found(xcb_window_t win)
{
	int i;

	for (i = 0; i < systray_count; i++)
		if (win == systray[i])
			return true;

	return false;
}

static void systray_remove(xcb_window_t win)
{
	xcb_window_t *tmp = malloc((systray_count - 1) * sizeof(xcb_window_t));
	int i, j = 0;

	for (i = 0; i < systray_count; i++) {
		if (win != systray[i]) {
			tmp[j] = systray[i];
			j++;
		}
	}

	systray_count--;
	free(systray);
	systray = tmp;
}

static void systray_setup(xcb_window_t win, xcb_client_message_event_t *ev)
{
	uint32_t values[2] = {24, 24};
	xcb_configure_window(conn, win,
			     XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
			     values);

	xcb_send_event(conn, 0, win, XCB_EVENT_MASK_NO_EVENT, (char *)ev);
	xcb_change_save_set(conn, XCB_SET_MODE_INSERT, win);

	xcb_reparent_window(conn, win, panel->id, 0, 0);
	xcb_map_window(conn, win);
	xcb_flush(conn);

	/* add this systray to the list */
	systray_count++;
	systray = (xcb_window_t *)realloc(systray,
					  systray_count * sizeof(xcb_window_t));
	systray[systray_count - 1] = win;
}

void panel_add_systray(xcb_client_message_event_t *ev)
{
	xcb_window_t win = ev->data.data32[2];

	if (ev->window != panel->id)
		return;

	if (systray_found(win) == true)
		return;

	systray_setup(win, ev);

	panel_draw();
}

void panel_remove_systray(xcb_unmap_notify_event_t *ev)
{
	xcb_window_t win = ev->window;

	if (systray_found(win) == true)
		systray_remove(win);

	panel_draw();
}

void panel_click(xcb_button_press_event_t *ev)
{
	int16_t x = ev->root_x;
	struct panel_client *panel_client;
	struct list *index;

	if (panel->id == ev->child) {
		for (index = panel_clients_head; index != NULL;
		     index = index->next) {
			panel_client = index->data;

			if (x > panel_client->pos
			    && x < (panel_client->pos + panel_client->width)) {
				window_raise(panel_client->client->id);
				client_set_focus(panel_client->client);
				break;
			}
		}
	}
}
