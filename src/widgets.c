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
#include <unistd.h>
#include <pthread.h>
#include <dlfcn.h>
#include <cairo/cairo-xcb.h>

#include "global.h"
#include "window.h"
#include "widgets.h"
#include "log.h"
#include "utils.h"
#include "conf.h"

struct widget_t {
	xcb_window_t panel;
	xcb_window_t win;
	int width;
	int height;
	cairo_surface_t *src;
	struct draw_t *draw;
	pthread_t thread;
	struct widget_module_t **modules;
	void **handles;
	int count;
};

static struct widget_t *widgets = NULL;

static void *widgets_loop(void __attribute__((__unused__)) * arg)
{
	struct widget_module_t *module = NULL;
	int i, pos;

	while (1) {
		LOGI("Refresh widgets");
		sleep(2);

		/* fill widgets black */
		struct area_t area = {0, 0, widgets->width, widgets->height};
		draw_set_color(widgets->draw, BLACK);
		draw_rectangle(widgets->draw, area, true);

		/* draw widgets */
		pos = 0;
		for (i = 0; i < widgets->count; i++) {
			module = widgets->modules[i];
			if (module && module->draw)
				module->draw(widgets->draw, &pos);
		}

		/* flush */
		cairo_surface_flush(widgets->src);
		xcb_flush(conn);
	}

	return 0;
}

static void widgets_load_modules(void)
{
	char **files = NULL;
	int i, count;
	void *handle = NULL;
	struct widget_module_t *module = NULL;

	/* read shared libraries */
	if (global_conf.widgets == NULL)
		return;

	files = file_in_dir(global_conf.widgets, &count);
	if (files == NULL)
		return;

	for (i = 0; i < count; i++) {
		/* open shared library */
		handle = dlopen(files[i], RTLD_NOW);
		if (handle == NULL) {
			LOGE("dlopen: %s\n", dlerror());
			continue;
		}

		/* search widget symbol */
		module = (struct widget_module_t *)dlsym(handle,
							 WIDGET_SYMBOL_STR);
		if (module == NULL) {
			dlclose(handle);
			LOGE("dlsym: %s\n", dlerror());
			continue;
		}

		/* add module/handle to widgets*/
		widgets->modules =
			realloc(widgets->modules,
				(widgets->count + 1)
					* sizeof(struct widget_module_t *));
		widgets->modules[widgets->count] = module;
		widgets->handles =
			realloc(widgets->handles,
				(widgets->count + 1) * sizeof(void *));
		widgets->handles[widgets->count] = handle;
		(widgets->count)++;
	}
}

static void widgets_unload_modules(void)
{
	void *handle = NULL;
	int i;

	for (i = 0; i < widgets->count; i++) {
		handle = widgets->handles[i];
		if (handle)
			dlclose(handle);
	}
}

static void widgets_init_modules(void)
{
	struct widget_module_t *module = NULL;
	int i;

	for (i = 0; i < widgets->count; i++) {
		module = widgets->modules[i];
		if (module && module->init)
			module->init();
	}
}

static void widgets_exit_modules(void)
{
	struct widget_module_t *module = NULL;
	int i;

	for (i = 0; i < widgets->count; i++) {
		module = widgets->modules[i];
		if (module && module->exit)
			module->exit();
	}
}

void widgets_init(xcb_window_t panel, int height)
{
	if (widgets)
		return;
	widgets = malloc(sizeof(struct widget_t));

	/* init values */
	widgets->modules = NULL;
	widgets->handles = NULL;
	widgets->count = 0;

	/* load/init widgets modules if found */
	widgets_load_modules();
	if (widgets->count == 0) {
		LOGI("No widgets modules found");
		free(widgets);
		widgets = NULL;
		return;
	}
	widgets_init_modules();

	/* create window */
	widgets->width = widgets_get_width();
	widgets->height = height;
	widgets->win = window_create(0, 0, widgets->width, widgets->height);
	xcb_reparent_window(conn, widgets->win, panel, 0, 0);
	window_show(widgets->win);

	/* init cairo */
	widgets->src = cairo_xcb_surface_create(
		conn, widgets->win, visual, widgets->width, widgets->height);
	widgets->draw = draw_create(widgets->src, "sans 12");

	/* start widgets loop */
	pthread_create(&widgets->thread, NULL, widgets_loop, NULL);
}

void widgets_exit(void)
{
	/* stop loop thread */
	pthread_cancel(widgets->thread);

	/* exit/unload/free widgets modules */
	widgets_exit_modules();
	widgets_unload_modules();
	free(widgets->modules);
	free(widgets->handles);

	/* free widgets window ressources */
	draw_destroy(widgets->draw);
	cairo_surface_destroy(widgets->src);

	TODO("Delete the window make segfault the proram")
	window_unmap(widgets->win);
	/* window_delete(widgets->win); */

	/* free widgets */
	free(widgets);
	widgets = NULL;
}

int widgets_get_width(void)
{
	struct widget_module_t *module = NULL;
	int i, width = 0;

	if (widgets == NULL)
		return 0;

	for (i = 0; i < widgets->count; i++) {
		module = widgets->modules[i];
		if (module)
			width += module->width;
	}

	return width;
}

xcb_window_t widgets_get_win(void)
{
	return widgets ? widgets->win : XCB_NONE;
}

void widgets_toggle(bool enable)
{
	if (widgets == NULL)
		return;

	if (enable == true) {
		window_show(widgets->win);
		pthread_create(&widgets->thread, NULL, widgets_loop, NULL);
	} else {
		window_unmap(widgets->win);
		pthread_cancel(widgets->thread);
	}
}

void widgets_reload(xcb_window_t panel, int height)
{
	LOGI("Reload widgets");

	if (widgets != NULL)
		widgets_exit();

	widgets_init(panel, height);
}
