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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cairo/cairo-xcb.h>

#include "global.h"
#include "monitor.h"
#include "window.h"
#include "client.h"
#include "utils.h"
#include "conf.h"
#include "panel.h"

/* list of all monitor */
struct list *monitors_head;

/* Beginning of RANDR extension events. */
int randrbase;

static struct monitor *monitor_add(xcb_randr_output_t id, char *name,
				   const int16_t x, const int16_t y,
				   const uint16_t width, const uint16_t height)
{
	struct list *index;
	struct monitor *mon;

	mon = malloc(sizeof(struct monitor));
	if (mon == NULL)
		return NULL;

	index = list_add(&monitors_head, mon);
	if (index == NULL)
		return NULL;

	mon->id = id;
	mon->name = name;
	mon->index = index;
	mon->x = x;
	mon->y = y;
	mon->width = width;
	mon->height = height;

	return mon;
}

static void monitor_remove(struct monitor *mon)
{
	list_remove(&monitors_head, mon->index);
}

static struct monitor *monitor_find_clones(xcb_randr_output_t id,
					   const int16_t x,
					   const int16_t y)
{
	struct monitor *clonemon;
	struct list *index;

	for (index = monitors_head; index != NULL; index = index->next) {
		clonemon = index->data;

		/* Check for same position. */
		if (id != clonemon->id && clonemon->x == x && clonemon->y == y)
			return clonemon;
	}

	return NULL;
}

static struct monitor *monitor_find_by_id(xcb_randr_output_t id)
{
	struct monitor *mon;
	struct list *index;

	for (index = monitors_head; index != NULL; index = index->next) {
		mon = index->data;

		if (id == mon->id)
			return mon;
	}

	return NULL;
}

struct monitor *monitor_find_by_coord(const int16_t x, const int16_t y)
{
	struct monitor *mon;
	struct list *index;

	for (index = monitors_head; index != NULL; index = index->next) {
		mon = index->data;

		if (x >= mon->x && x <= mon->x + mon->width && y >= mon->y && y
		    <= mon->y + mon->height)
			return mon;
	}

	/* Window coordinates are outside all physical monitors.
	 * Choose the first screen.*/
	return monitors_head->data;
}

static struct monitor *monitor_get_first_from_head(void)
{
	struct monitor *monitor = NULL;

	if (monitors_head != NULL && monitors_head->data != NULL) {
		monitor = monitors_head->data;
		while (monitor->index->next != NULL)
			monitor = monitor->index->next->data;
	}

	return monitor;
}

static void monitor_check_client(struct client *cl)
{
	struct monitor *mon;
	struct list *index_monitor;
	struct monitor *first_monitor = monitor_get_first_from_head();

	bool test = false;

	/* loop through monitors */
	for (index_monitor = monitors_head; index_monitor != NULL; index_monitor = index_monitor->next) {
		mon = index_monitor->data;

		if (cl->monitor == mon)
			test = true;
	}

	/* monitor of this client no found in monitors list
	   assign it to the first monitor  */
	if (test == false) {
		cl->monitor = first_monitor;
		client_fit_on_screen(cl);
	}
	test = false;
}

static void monitor_handle_output(xcb_randr_output_t id, xcb_randr_get_output_info_reply_t *output, xcb_timestamp_t timestamp)
{
	xcb_randr_get_crtc_info_cookie_t cookie;
	xcb_randr_get_crtc_info_reply_t *crtc = NULL;
	struct monitor *mon, *clonemon, *new;
	int name_len;
	char *name;

	/* check crtc and physical output connected */
	if ((output->crtc != XCB_NONE) && (output->connection == XCB_RANDR_CONNECTION_CONNECTED)) {

		/* get crtc info */
		cookie = xcb_randr_get_crtc_info(conn, output->crtc, timestamp);
		crtc = xcb_randr_get_crtc_info_reply(conn, cookie, NULL);
		if (crtc == NULL)
			return;

		/* check if it's a clone. */
		clonemon = monitor_find_clones(id, crtc->x, crtc->y);
		if (clonemon != NULL)
			return;

		/* check if monitor already in the list */
		mon = monitor_find_by_id(id);
		if (mon == NULL) {
			/* get name of monitor */
			name_len = MIN(16, xcb_randr_get_output_info_name_length(output));
			name = malloc(name_len + 1);
			snprintf(name, name_len + 1, "%.*s", name_len, xcb_randr_get_output_info_name(output));

			/* add to the list */
			monitor_add(id, name, crtc->x, crtc->y, crtc->width, crtc->height);
		} else
			/* We know this monitor. Update information.
			 * If it's smaller than before, rearrange windows. */
			if (crtc->x != mon->x || crtc->y != mon->y ||
			    crtc->width != mon->width || crtc->height != mon->height) {
				if (crtc->x != mon->x)
					mon->x = crtc->x;
				if (crtc->y != mon->y)
					mon->y = crtc->y;
				if (crtc->width != mon->width)
					mon->width = crtc->width;
				if (crtc->height != mon->height)
					mon->height = crtc->height;

				client_monitor_updated(mon);
			}
		free(crtc);
	} else {
		/* move clients from this monitor and remove it */
		mon = monitor_find_by_id(id);
		if (mon) {
			new = monitor_get_first_from_head();
			if (new)
				client_monitor_reassign(mon, new);

			monitor_remove(mon);
		}
	}
}

void monitor_set_wallpaper(void)
{
	struct monitor *mon;
	struct list *index;
	float scale_width, scale_height;

	/* check if we can access wallpaper path */
	if ((global_conf.wallpaper == NULL) || (file_access(global_conf.wallpaper) == false))
		return;

	/* create a pixmap  */
	xcb_pixmap_t p = xcb_generate_id(conn);
	uint16_t width = screen->width_in_pixels;
	uint16_t height = screen->height_in_pixels;
	xcb_create_pixmap(conn, screen->root_depth, p, screen->root, width, height);

	/* create surface from png file */
	cairo_surface_t *src = cairo_image_surface_create_from_png(global_conf.wallpaper);
	int image_height = cairo_image_surface_get_height(src);
	int image_width = cairo_image_surface_get_width(src);

	/* loop through monitors and paint the scaled image */
	cairo_surface_t *dest = cairo_xcb_surface_create(conn, p, visual, width, height);
	cairo_t *cr = cairo_create(dest);
	for (index = monitors_head; index != NULL; index = index->next) {
		mon = index->data;

		scale_width = ((float) mon->width) / ((float) image_width);
		scale_height = ((float) mon->height) / ((float) image_height);

		cairo_scale(cr, scale_width, scale_height);
		cairo_set_source_surface(cr, src, mon->x / scale_width, mon->y / scale_height);
		cairo_paint(cr);
		cairo_scale(cr, 1 / scale_width, 1 / scale_height);
	}
	cairo_surface_flush(dest);

	/* change root window background pixmap */
	xcb_change_window_attributes(conn, screen->root, XCB_CW_BACK_PIXMAP, &p);
	xcb_clear_area(conn, 0, screen->root, 0, 0, 0, 0);
	xcb_flush(conn);

	/* free resources */
	xcb_free_pixmap(conn, p);
	cairo_surface_destroy(src);
	cairo_surface_destroy(dest);
	cairo_destroy(cr);
}

static void monitor_update(void)
{
	int i, len;
	xcb_randr_get_screen_resources_current_cookie_t rcookie;
	xcb_randr_get_screen_resources_current_reply_t *res;
	xcb_randr_get_output_info_cookie_t ocookie;
	xcb_randr_get_output_info_reply_t *output;
	xcb_randr_output_t *outputs;
	xcb_timestamp_t timestamp;

	/* get screen resources */
	rcookie = xcb_randr_get_screen_resources_current(conn, screen->root);
	res = xcb_randr_get_screen_resources_current_reply(conn, rcookie, NULL);
	if (res == NULL)
		return;

	/* Request information for all outputs. */
	outputs = xcb_randr_get_screen_resources_current_outputs(res);
	len = xcb_randr_get_screen_resources_current_outputs_length(res);
	timestamp = res->config_timestamp;

	/* loop though all outputs */
	for (i = 0; i < len; i++) {
		ocookie = xcb_randr_get_output_info(conn, outputs[i], timestamp);
		output = xcb_randr_get_output_info_reply(conn, ocookie, NULL);
		if (output != NULL) {
			monitor_handle_output(outputs[i], output, timestamp);
			free(output);
		}
	}

	/* TODO: do we need to do this everytime ???? */
	panel_update_geom();
	client_foreach(monitor_check_client);

	/* free resources */
	free(res);

	/* set wallpaper on all monitors */
	monitor_set_wallpaper();
}

void monitor_init(void)
{
	const xcb_query_extension_reply_t *extension = xcb_get_extension_data(conn, &xcb_randr_id);

	if (!extension->present)
		randrbase = -1;
	else
		monitor_update();

	randrbase = extension->first_event;
	xcb_randr_select_input(conn, screen->root,
			       XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE
			       | XCB_RANDR_NOTIFY_MASK_OUTPUT_CHANGE
			       | XCB_RANDR_NOTIFY_MASK_CRTC_CHANGE
			       | XCB_RANDR_NOTIFY_MASK_OUTPUT_PROPERTY);
}

void monitor_event(uint8_t response_type)
{
	if (response_type == randrbase + XCB_RANDR_SCREEN_CHANGE_NOTIFY)
		monitor_update();
}

void monitor_borders(int16_t *x, int16_t *y, uint16_t *width, uint16_t *height)
{
	struct monitor *mon;
	struct list *index;
	int16_t min_x = INT16_MAX, min_y = INT16_MAX;
	uint16_t max_width = 0, max_height = 0;

	for (index = monitors_head; index != NULL; index = index->next) {
		mon = index->data;

		/* set minimum value in X */
		if (mon->x < min_x)
			min_x = mon->x;

		/* set minimum value in Y */
		if (mon->y < min_y)
			min_y = mon->y;

		/* set max value of width */
		max_width += mon->width;

		/* set max value of height */
		if (mon->height > max_height)
			max_height = mon->height;
	}

	/* assign values */
	*x = min_x;
	*y = min_y;
	*width = max_width;
	*height = max_height;
}
