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

#ifndef ACTION_H
#define ACTION_H

enum { WIN_MOVE, WIN_RESIZE };
enum { MAXHALF_VERTICAL_RIGHT, MAXHALF_VERTICAL_LEFT };
enum { FULLSCREEN_ONE_MONITOR, FULLSCREEN_ALL_MONITOR };

typedef union {
	const char **	com;
	const int8_t	i;
} Arg;

void change_focus(const Arg *arg);
void max_half(const Arg *arg);
void delete_window(const Arg *arg);
void maximize(const Arg *arg);
void hide(const Arg *arg);
void raise_all(const Arg *arg);
void start(const Arg *arg);
void jwm_exit(const Arg *arg);
void mouse_motion(const Arg *arg);
void reload_conf(const Arg *arg);

#endif
