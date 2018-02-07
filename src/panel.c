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

#define PANEL_FONT "sans 12"
#define PANEL_REFRESH 60
#define PANEL_HEIGHT 30
#define PANEL_TEXT_SIZE 11

struct panel *panel = NULL;

struct panel_client_data {
	struct monitor	*mon;
	double		*pos;
	double		*max_width;
};

enum color_t {
	NORMAL,
	ORANGE
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

	/* init pango */
	panel->layout = pango_cairo_create_layout(panel->cr);
	panel->font = pango_font_description_from_string(PANEL_FONT);
	pango_layout_set_font_description(panel->layout, panel->font);

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

		/* set new pango values */
		g_object_unref(panel->layout);
		panel->layout = pango_cairo_create_layout(panel->cr);
		pango_layout_set_font_description(panel->layout, panel->font);

		/* move, resize and show panel */
		window_move_resize(panel->id, border_x, border_y, border_width, PANEL_HEIGHT);
		panel_draw();
	}
}

static int panel_get_text_width(char *text, size_t len)
{
	int width;
	PangoLayout *tmp = pango_cairo_create_layout(panel->cr);

	pango_layout_set_text(tmp, text, len);
	pango_layout_get_pixel_size(tmp, &width , NULL);
	g_object_unref(tmp);

	return width;
}

static void panel_draw_text(double x, int width, char *text, size_t len, enum color_t color)
{
	int height;
	PangoLayout *tmp = pango_cairo_create_layout(panel->cr);
	pango_layout_set_font_description(tmp, panel->font);

	/* set text and get size */
	pango_layout_set_text(tmp, text, len);
	pango_layout_get_pixel_size(tmp, NULL, &height);

	/* set color */
	switch (color) {
	case ORANGE:
		cairo_set_source_rgb(panel->cr, 0.98, 0.52, 0.07);
		break;
	case NORMAL:
		cairo_set_source_rgb(panel->cr, 0.5, 0.5, 0.5);
		break;
	}
	pango_cairo_update_layout(panel->cr, panel->layout);

	/* set geometry */
	pango_layout_set_width(tmp, width * PANGO_SCALE);
	pango_layout_set_height(tmp, panel->height * PANGO_SCALE);
	pango_layout_set_alignment(tmp, PANGO_ALIGN_CENTER);

	/* move and show layout */
	cairo_move_to(panel->cr, x, (PANEL_HEIGHT - height) / 2);
	pango_cairo_show_layout(panel->cr, tmp);
	g_object_unref(tmp);
}

static void draw_clock(double *max_width)
{
	time_t t;
	struct tm *lt;
	char buf_time[256];
	int width;

	/* get current date */
	t = time(NULL);
	lt = localtime(&t);
	buf_time[strftime(buf_time, sizeof(buf_time), "%R  -  %d %b", lt)] = '\0';

	/* Get width and update max width */
	width = panel_get_text_width(buf_time, strlen(buf_time));
	width += 5;
	*max_width = panel->width - width;

	/* draw text */
	panel_draw_text(*max_width, width, buf_time, strlen(buf_time), NORMAL);
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
	snprintf(icon_path, 256, "%s%s.png", ICONS_DIR, name);
	if (file_access(icon_path) == false)
		snprintf(icon_path, 256, "%sdefault.png", ICONS_DIR);
}

static void get_window_name(xcb_window_t win, char *name, int len)
{
	xcb_get_property_cookie_t cookie;
	xcb_ewmh_get_utf8_strings_reply_t data;

	cookie = xcb_ewmh_get_wm_name(ewmh, win);
	if (xcb_ewmh_get_wm_name_reply(ewmh, cookie, &data, NULL))
		strncpy(name, data.strings, data.strings_len);
}

static void panel_draw_rectangle(double x, double y, double width, double height)
{
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

	cairo_set_line_width(panel->cr, 1);
	cairo_stroke(panel->cr);
}

static void draw_client(struct client *client, void *data)
{
	struct client *focus = client_get_focus();
	struct panel_client_data *client_data = (struct panel_client_data *)data;
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

	/* draw rectangle, icon and name of the process */
	if (strlen(name) > 0) {

		width_name = panel_get_text_width(name, strlen(name));

		/* check if we can draw this client */
		if ((*client_data->pos + (width_name + 15 + 24) > *client_data->max_width) ||
		    (*client_data->pos + (width_name + 15 + 24) > client->monitor->width + client->monitor->x))
			return;

		/* rounded rectangle around name */
		if ((client == focus) && (client->iconic == false))
			cairo_set_source_rgb(panel->cr, 0.98, 0.52, 0.07);
		else
			cairo_set_source_rgb(panel->cr, 0.5, 0.5, 0.5);
		panel_draw_rectangle(*client_data->pos, 0, width_name + 15 + 24, PANEL_HEIGHT);

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
			panel_draw_text(*client_data->pos, width_name, name, strlen(name), ORANGE);
		else
			panel_draw_text(*client_data->pos, width_name, name, strlen(name), NORMAL);

		/* update position if next client */
		if (client->index->next != NULL)
			*client_data->pos += width_name + 5 + 4;
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
