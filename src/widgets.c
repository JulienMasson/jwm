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

#include "global.h"
#include "window.h"
#include "widgets.h"
#include "log.h"

struct widget_t {
	xcb_window_t win;
	pthread_t thread;
};

struct widget_t *widgets = NULL;

static void *widgets_loop(void __attribute__((__unused__)) * arg)
{
	while (1) {
		LOGI("Refresh widgets");
		sleep(2);
	}

	return 0;
}

void widgets_init(void)
{
	if (widgets)
		return;

	widgets = malloc(sizeof(struct widget_t));

	widgets->win = window_create(0, 0, 0, 0);
	pthread_create(&widgets->thread, NULL, widgets_loop, NULL);
}

int widgets_get_width(void)
{
	return 0;
}

xcb_window_t widgets_get_win(void)
{
	return widgets->win;
}

void widgets_toggle(bool enable)
{
	if (enable == true) {
		window_show(widgets->win);
		pthread_create(&widgets->thread, NULL, widgets_loop, NULL);
	} else {
		window_unmap(widgets->win);
		pthread_cancel(widgets->thread);
	}
}

void widgets_reload(void)
{
	LOGI("Reload widgets");
}
