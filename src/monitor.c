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

#include "global.h"
#include "monitor.h"
#include "window.h"
#include "client.h"
#include "utils.h"

/* List of all physical monitor outputs. */
struct list *monlist;

/* Beginning of RANDR extension events. */
int randrbase;

static struct monitor *monitor_add(xcb_randr_output_t id, char *name,
				   const int16_t x, const int16_t y,
				   const uint16_t width, const uint16_t height)
{
	struct list *element;
	struct monitor *mon;

	mon = malloc(sizeof(struct monitor));
	if (mon == NULL)
		return NULL;

	element = list_add(&monlist, mon);
	if (element == NULL)
		return NULL;

	mon->id = id;
	mon->name = name;
	mon->element = element;
	mon->x = x;
	mon->y = y;
	mon->width = width;
	mon->height = height;

	return mon;
}

static void monitor_remove(struct monitor *mon)
{
	list_remove(&monlist, mon->element);
}

static struct monitor *monitor_find_clones(xcb_randr_output_t id,
					   const int16_t x,
					   const int16_t y)
{
	struct monitor *clonemon;
	struct list *element;

	for (element = monlist; element != NULL; element = element->next) {
		clonemon = element->data;

		/* Check for same position. */
		if (id != clonemon->id && clonemon->x == x && clonemon->y == y)
			return clonemon;
	}

	return NULL;
}

static struct monitor *monitor_find_by_id(xcb_randr_output_t id)
{
	struct monitor *mon;
	struct list *element;

	for (element = monlist; element != NULL; element = element->next) {
		mon = element->data;

		if (id == mon->id)
			return mon;
	}

	return NULL;
}

struct monitor *monitor_find_by_coord(const int16_t x, const int16_t y)
{
	struct monitor *mon;
	struct list *element;

	for (element = monlist; element != NULL; element = element->next) {
		mon = element->data;

		if (x >= mon->x && x <= mon->x + mon->width && y >= mon->y && y
		    <= mon->y + mon->height)
			return mon;
	}

	/* Window coordinates are outside all physical monitors.
	 * Choose the first screen.*/
	return monlist->data;
}

static void monitor_reaarange_windows(struct monitor *monitor)
{
	struct client *client;
	struct list *element;

	for (element = winlist; element != NULL; element = element->next) {
		client = element->data;

		if (client->monitor == monitor)
			window_fitonscreen(client);
	}
}

/* Walk through all the RANDR outputs (number of outputs == len) there */
static void getoutputs(xcb_randr_output_t *outputs, const int len,
		       xcb_timestamp_t timestamp)
{
	int i;
	int name_len;
	char *name;

	/* was at time timestamp. */
	xcb_randr_get_crtc_info_cookie_t icookie;
	xcb_randr_get_crtc_info_reply_t *crtc = NULL;
	xcb_randr_get_output_info_reply_t *output;
	struct monitor *mon, *clonemon;
	struct list *element;
	xcb_randr_get_output_info_cookie_t ocookie[len];

	for (i = 0; i < len; i++)
		ocookie[i] = xcb_randr_get_output_info(conn, outputs[i],
						       timestamp);

	/* Loop through all outputs. */
	for (i = 0; i < len; i++) {
		if ((output = xcb_randr_get_output_info_reply(conn, ocookie[i],
							      NULL)) == NULL)
			continue;

		name_len = MIN(16, xcb_randr_get_output_info_name_length(output));
		name = malloc(name_len + 1);

		snprintf(name, name_len + 1, "%.*s", name_len,
			 xcb_randr_get_output_info_name(output));

		if (output->crtc != XCB_NONE) {
			icookie = xcb_randr_get_crtc_info(conn, output->crtc,
							  timestamp);
			crtc = xcb_randr_get_crtc_info_reply(conn, icookie, NULL);
			if (crtc == NULL)
				return;

			/* Check if it's a clone. */
			clonemon = monitor_find_clones(outputs[i], crtc->x, crtc->y);
			if (clonemon != NULL)
				continue;

			/* Do we know this monitor already? */
			if (NULL == (mon = monitor_find_by_id(outputs[i]))) {
				monitor_add(outputs[i], name, crtc->x, crtc->y,
					   crtc->width, crtc->height);
			} else
				/* We know this monitor. Update information.
				 * If it's smaller than before, rearrange windows. */
				if (crtc->x != mon->x || crtc->y != mon->y || crtc->width
				    != mon->width || crtc->height
				    != mon->height) {
					if (crtc->x != mon->x)
						mon->x = crtc->x;
					if (crtc->y != mon->y)
						mon->y = crtc->y;
					if (crtc->width != mon->width)
						mon->width = crtc->width;
					if (crtc->height != mon->height)
						mon->height = crtc->height;

					monitor_reaarange_windows(mon);
				}
			free(crtc);
		} else {
			/* Check if it was used before. If it was, do something. */
			if ((mon = monitor_find_by_id(outputs[i]))) {
				struct client *client;

				for (element = winlist; element != NULL; element = element->next) {
					/* Check all windows on this monitor
					 * and move them to the next or to the
					 * first monitor if there is no next. */
					client = element->data;

					if (client->monitor == mon) {
						if (client->monitor->element->next == NULL) {
							if (monlist == NULL)
								client->monitor = NULL;
							else
								client->monitor = monlist->data;
						} else {
							client->monitor = client->monitor->element->next->data;
						}
						window_fitonscreen(client);
					}
				}

				/* It's not active anymore. Forget about it. */
				monitor_remove(mon);
			}
		}
		if (output != NULL)
			free(output);

		free(name);
	} /* for */
}

/* Get RANDR resources and figure out how many outputs there are. */
static void monitor_update(void)
{
	int len;

	xcb_randr_get_screen_resources_current_cookie_t rcookie
		= xcb_randr_get_screen_resources_current(conn, screen->root);
	xcb_randr_get_screen_resources_current_reply_t *res
		= xcb_randr_get_screen_resources_current_reply(conn, rcookie,
							       NULL);

	if (res == NULL)
		return;

	xcb_timestamp_t timestamp = res->config_timestamp;

	len = xcb_randr_get_screen_resources_current_outputs_length(res);
	xcb_randr_output_t *outputs
		= xcb_randr_get_screen_resources_current_outputs(res);

	/* Request information for all outputs. */
	getoutputs(outputs, len, timestamp);
	free(res);
}

/* Set up RANDR extension. Get the extension base and subscribe to events */
void monitor_init(void)
{
	const xcb_query_extension_reply_t *extension
		= xcb_get_extension_data(conn, &xcb_randr_id);

	if (!extension->present)
		randrbase = -1;
	else
		monitor_update();

	randrbase = extension->first_event;
	xcb_randr_select_input(conn, screen->root,
			       XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE
			       | XCB_RANDR_NOTIFY_MASK_OUTPUT_CHANGE
			       | XCB_RANDR_NOTIFY_MASK_CRTC_CHANGE
			       | XCB_RANDR_NOTIFY_MASK_OUTPUT_PROPERTY
		);

}

void monitor_event(uint8_t response_type)
{
	if (response_type == randrbase + XCB_RANDR_SCREEN_CHANGE_NOTIFY)
		monitor_update();
}

void monitor_borders(uint16_t *border_x, uint16_t *border_y)
{
	struct monitor *mon;
	struct list *element;
	uint16_t x = 0, width = 0, height = 0;

	for (element = monlist; element != NULL; element = element->next) {
		mon = element->data;
		if (mon->x + mon->width > x + width) {
			x = mon->x;
			width = mon->width;
		}
		if (mon->height > height)
			height = mon->height;
	}

	/* assign values */
	*border_x = x + width;
	*border_y = height;
}
