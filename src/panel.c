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
#include <math.h>

#include "global.h"
#include "panel.h"
#include "window.h"
#include "monitor.h"
#include "client.h"
#include "utils.h"

#define PANEL_REFRESH 60
#define PANEL_HEIGHT 30
#define PANEL_TEXT_SIZE 11

struct panel *panel = NULL;

struct panel_client_data {
	struct monitor	*mon;
	double		*pos;
	double		*max_width;
};

void panel_init(void)
{
	int16_t border_x, border_y;
	uint16_t border_width, border_height;

	monitor_borders(&border_x, &border_y, &border_width, &border_height);
	panel = malloc(sizeof(struct panel));

	/* init panel values */
	panel->id = window_create(border_x, border_y, border_width, PANEL_HEIGHT);
	panel->x = border_x;
	panel->y = border_y;
	panel->width = border_width;
	panel->height = PANEL_HEIGHT;
	panel->enable = true;
	panel->refresh = PANEL_REFRESH;

	/* init cairo */
	panel->src = cairo_xcb_surface_create(conn, panel->id, visual,
					      panel->width - panel->x,
					      panel->height - panel->y);
	panel->cr = cairo_create(panel->src);
	cairo_select_font_face(panel->cr, "DejaVu Sans Mono Book", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(panel->cr, PANEL_TEXT_SIZE);

	/* show panel */
	window_show(panel->id);
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

	if (panel && ((border_x != panel->x) ||
		      (border_y != panel->y) ||
		      (border_width != panel->width))) {
		/* set new values */
		panel->x = border_x;
		panel->y = border_y;
		panel->width = border_width;

		/* set new cairo values */
		cairo_surface_destroy(panel->src);
		cairo_destroy(panel->cr);
		panel->src = cairo_xcb_surface_create(conn, panel->id, visual,
						      panel->width - panel->x,
						      panel->height - panel->y);
		panel->cr = cairo_create(panel->src);

		/* move, resize and show panel */
		window_move_resize(panel->id, border_x, border_y, border_width, PANEL_HEIGHT);
		panel_draw();
	}
}

static void draw_clock(double *max_width)
{
	cairo_text_extents_t extents;
	time_t t;
	struct tm *lt;
	char date[30];

	/* get current date */
	t = time(NULL);
	lt = localtime(&t);
	date[strftime(date, sizeof(date), "%R  -  %d %b", lt)] = '\0';

	/* paint it in cairo */
	cairo_set_source_rgb(panel->cr, 0.5, 0.5, 0.5);
	cairo_text_extents(panel->cr, date, &extents);
	*max_width = panel->width - extents.width - 5;
	cairo_move_to(panel->cr, *max_width, PANEL_HEIGHT - ((PANEL_HEIGHT - extents.height) / 2));
	cairo_show_text(panel->cr, date);
}

static void get_icon_path(char *process_name, char *icon_path)
{
	if (strncmp(process_name, "urxvt", 256) == 0)
		snprintf(icon_path, 256, "/usr/share/icons/gnome/24x24/apps/terminal.png");
	else if (strncmp(process_name, "firefox", 256) == 0)
		snprintf(icon_path, 256, "/usr/share/icons/gnome/24x24/apps/firefox.png");
	else if (strncmp(process_name, "emacs", 256) == 0)
		snprintf(icon_path, 256, "/home/lab/bin/emacs-repo/etc/images/icons/hicolor/24x24/apps/emacs.png");
	else if (strncmp(process_name, "gnome-system-monitor", 256) == 0)
		snprintf(icon_path, 256, "/usr/share/icons/gnome/24x24/apps/gnome-monitor.png");
	else if (strncmp(process_name, "Thunar", 256) == 0)
		snprintf(icon_path, 256, "/usr/share/icons/gnome/24x24/apps/file-manager.png");
	else
		snprintf(icon_path, 256, "/usr/share/icons/gnome/24x24/status/error.png");
}

static void draw_client(struct client *client, void *data)
{
	cairo_text_extents_t extents;
	xcb_get_property_cookie_t cookie;
	struct client *focus = client_get_focus();
	struct panel_client_data *client_data = (struct panel_client_data *)data;
	char process_name[256];
	char icon_path[256];
	uint32_t pid;

	/* check monitor */
	if (client->monitor != client_data->mon)
		return;

	/* get icon from process name of the window */
	memset(process_name, '\0', 256);
	cookie = xcb_ewmh_get_wm_pid(ewmh, client->id);
	if (xcb_ewmh_get_wm_pid_reply(ewmh, cookie, &pid, NULL))
		get_process_name(pid, process_name, 256);
	get_icon_path(process_name, icon_path);

	/* draw rectangle, icon and name of the process */
	if (strlen(process_name) > 0) {

		/* set extents */
		cairo_text_extents(panel->cr, process_name, &extents);

		/* check if we can draw this client */
		if ((*client_data->pos + (extents.width + 15 + 24) > *client_data->max_width) ||
		    (*client_data->pos + (extents.width + 15 + 24) > client->monitor->width + client->monitor->x))
			return;

		/* rounded rectangle around name */
		double	x	      = *client_data->pos;
		double	y	      = 0;
		double	width         = extents.width + 15 + 24;
		double	height        = PANEL_HEIGHT;
		double	aspect        = 1.0;
		double	corner_radius = height / 10.0;
		double	radius	      = corner_radius / aspect;
		double	degrees	      = M_PI / 180.0;

		cairo_new_sub_path(panel->cr);
		cairo_arc(panel->cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
		cairo_arc(panel->cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
		cairo_arc(panel->cr, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
		cairo_arc(panel->cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
		cairo_close_path(panel->cr);

		if ((client == focus) && (client->iconic == false))
			cairo_set_source_rgb(panel->cr, 0.98, 0.52, 0.07);
		else
			cairo_set_source_rgb(panel->cr, 0.5, 0.5, 0.5);
		cairo_set_line_width(panel->cr, 1);
		cairo_stroke(panel->cr);

		/* shift to display icon */
		*client_data->pos += 5;

		/* draw icon */
		cairo_surface_t *image = cairo_image_surface_create_from_png(icon_path);
		cairo_set_source_surface(panel->cr, image, *client_data->pos, 3);
		cairo_paint(panel->cr);
		cairo_surface_destroy(image);

		/* shift to show name */
		*client_data->pos += 24 + 5;

		/* show client name */
		if ((client == focus) && (client->iconic == false))
			cairo_set_source_rgb(panel->cr, 0.98, 0.52, 0.07);
		else
			cairo_set_source_rgb(panel->cr, 0.5, 0.5, 0.5);
		cairo_move_to(panel->cr, *client_data->pos, PANEL_HEIGHT - ((PANEL_HEIGHT - extents.height) / 2));
		cairo_show_text(panel->cr, process_name);

		/* update position if next client */
		if (client->index->next != NULL)
			*client_data->pos += extents.width + 5 + 4;
	}
}

static void draw_by_monitor(struct monitor *mon, void *data)
{
	struct panel_client_data *client_data = (struct panel_client_data *)data;
	client_data->mon = mon;
	*client_data->pos = mon->x + 1;

	client_foreach(draw_client, (void *)client_data);
}

void panel_draw(void)
{
	double pos = 0;
	double max_width = 0;
	struct panel_client_data client_data = { NULL, &pos, &max_width};

	if (panel->enable == true) {
		/* fill panel black */
		cairo_set_source_rgb(panel->cr, 0, 0, 0);
		cairo_rectangle(panel->cr, panel->x, panel->y, panel->width, panel->height);
		cairo_fill(panel->cr);

		/* draw clock and get max_width */
		draw_clock(client_data.max_width);

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
