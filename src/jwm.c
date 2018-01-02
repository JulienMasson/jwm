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

#include <stdbool.h>
#include <stdio.h>
#include <getopt.h>
#include "global.h"
#include "log.h"
#include "ewmh.h"
#include "monitor.h"
#include "input.h"
#include "atom.h"
#include "event.h"
#include "conf.h"
#include "cursor.h"
#include <cairo/cairo-xcb.h>
#include <xcb/xcb_aux.h>

/* global vars */
xcb_connection_t *conn;                 /* Connection to X server. */
xcb_screen_t *screen;                   /* Our current screen. */
struct list *winlist;                   /* Global list of all client windows. */
struct client *focuswin;                /* Current focus window. */

void cleanup(void)
{
	ewmh_exit();
	xcb_set_input_focus(conn, XCB_NONE, XCB_INPUT_FOCUS_POINTER_ROOT,
			    XCB_CURRENT_TIME);
	xcb_flush(conn);
	xcb_disconnect(conn);
}

static void set_wallpaper(int scrno)
{
    /* create a pixmap  */
    xcb_pixmap_t p = xcb_generate_id(conn);
    uint16_t width = screen->width_in_pixels;
    uint16_t height = screen->height_in_pixels;
    xcb_create_pixmap(conn, screen->root_depth, p, screen->root, width, height);

    /* create surface for wallpaper */
    cairo_surface_t *src = cairo_image_surface_create_from_png(global_conf.wallpaper);
    int image_height = cairo_image_surface_get_height(src);
    int image_width = cairo_image_surface_get_width(src);

    /* create surface for root screen and paint the wallpaper */
    xcb_visualtype_t *visual = xcb_aux_get_visualtype(conn, scrno, screen->root_visual);
    cairo_surface_t *dest = cairo_xcb_surface_create(conn, p, visual, width, height);
    cairo_t *cr = cairo_create(dest);
    float scale_width = ((float) width) / ((float) image_width);
    float scale_height = ((float) height) / ((float) image_height);
    cairo_scale(cr, scale_width, scale_height);
    cairo_set_source_surface(cr, src, 0, 0);
    cairo_paint(cr);
    cairo_surface_flush(dest);

    /* Change the wallpaper */
    xcb_change_window_attributes(conn, screen->root, XCB_CW_BACK_PIXMAP, &p);
    xcb_clear_area(conn, 0, screen->root, 0, 0, 0, 0);
    xcb_flush(conn);

    /* free resources */
    xcb_free_pixmap(conn, p);
    cairo_destroy(cr);
    cairo_surface_destroy(src);
    cairo_surface_destroy(dest);
}

static bool init(int scrno)
{
	/* get screen */
	screen = xcb_aux_get_screen(conn, scrno);
	if (!screen) {
		LOGE("Get screen of display failed");
		return false;
	}

	/* wallpaper */
	if (global_conf.wallpaper)
	    set_wallpaper(scrno);

	/* init events */
	if (!event_init()) {
		LOGE("Event init failed");
		return false;
	}

	/* ewmh init */
	ewmh_init(scrno);

	/* atom init */
	atom_init();

	/* init all monitors */
	monitor_init();

	/* init input, keyboard, button ... */
	if (!input_init())
		return false;

	/* init cursor */
	cursor_init();

	return true;
}

void usage(void)
{
	printf("NAME\n"
	       "       jwm - A minimalist floating WM, written over the XCB\n"
	       "\n"
	       "SYNOPSIS\n"
	       "       jwm [OPTION...]\n"
	       "\n"
	       "OPTIONS\n"
	       "       -c, --conf\n"
	       "              Specifies which configuration file to use instead of the default.\n"
	       "\n"
	       "       -h, --help\n"
	       "              Display this help and exits.\n"
	       "\n");
}

int main(int argc, char **argv)
{
	int screenp, ch;
	char *conf_file = NULL;
	struct option long_options[] = {
		{"conf", required_argument, NULL, 'c'},
		{"help", no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0}
	};

	/* parse args */
	while ((ch = getopt_long(argc, argv, "c:h:", long_options, NULL)) != -1) {
		switch (ch) {
		case 'c':
			conf_file = optarg;
			break;
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
			break;
		}
	}

	/* init log and load global conf */
	conf_init(conf_file);
	log_init();

	/* call cleanup on normal process termination */
	atexit(cleanup);

	/* init X connection and start main loop */
	LOGI("Start connection with X server");
	conn = xcb_connect(NULL, &screenp);
	if (xcb_connection_has_error(conn))
		LOGE("Connection has shut down due to a fatal error");
	else
		if (init(screenp)) {
			LOGI("Start main loop");
			event_loop();
		}

	/* exit from the main loop */
	LOGW("Exit main loop");
	event_exit();
}
