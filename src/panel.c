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

#include "global.h"
#include "panel.h"
#include "window.h"
#include "monitor.h"

#define PANEL_HEIGHT 120

struct panel panel;

void panel_init(void)
{
	uint16_t border_x, border_y;
	monitor_borders(&border_x, &border_y);

	/* init panel values */
	panel.id = window_create(0, 0, border_x, PANEL_HEIGHT);
	panel.x = 0;
	panel.y = 0;
	panel.width = border_x;
	panel.height = PANEL_HEIGHT;
	panel.enable = true;

	/* show panel */
	window_show(panel.id);
}

struct panel *panel_get(void)
{
	return &panel;
}
