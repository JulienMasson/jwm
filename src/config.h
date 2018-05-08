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

#ifndef CONFIG_H
#define CONFIG_H

/* Programs */
static const char *rofi_run[] = {"rofi",   "-show",       "run",
				 "-theme", "flat-orange", NULL};
static const char *urxvt[] = {"urxvt", NULL};
static const char *emacs[] = {"emacs", NULL};
static const char *volume_up[] = {"amixer", "set", "Master", "4%+", NULL};
static const char *volume_down[] = {"amixer", "set", "Master", "4%-", NULL};
static const char *volume_toggle[] = {"pactl", "set-sink-mute", "0", "toggle",
				      NULL};
static const char *i3lock[] = {"i3lock-fancy", NULL};

/* Keyboard shorcut */
static key keys[] = {
	/* modifier      key        function           argument */
	/* Focus to next/previous window */
	{MOD, XK_Right, change_focus, {.i = CLIENT_NEXT}},
	{MOD, XK_Left, change_focus, {.i = CLIENT_PREVIOUS}},
	/* Vertically left/right */
	{MOD | CONTROL, XK_Right, max_half, {.i = MAXHALF_VERTICAL_RIGHT}},
	{MOD | CONTROL, XK_Left, max_half, {.i = MAXHALF_VERTICAL_LEFT}},
	/* Kill a window */
	{MOD | SHIFT, XK_c, delete_window, {}},
	/* Full screen window without borders */
	{MOD, XK_f, maximize, {.i = FULLSCREEN_ONE_MONITOR}},
	{MOD | CONTROL, XK_t, maximize, {.i = FULLSCREEN_ALL_MONITOR}},
	/* Hide / Raise windows */
	{MOD, XK_h, hide, {}},
	{MOD, XK_a, raise_all, {}},
	/* Reload conf */
	{MOD, XK_r, reload_conf, {}},
	/* Toggle panel */
	{MOD, XK_p, panel_toggle, {}},
	/* Programs */
	{MOD, XK_d, start, {.com = rofi_run}},
	{MOD, XK_Return, start, {.com = urxvt}},
	{MOD | SHIFT, XK_e, start, {.com = emacs}},
	{MOD, XK_i, start, {.com = volume_up}},
	{MOD, XK_u, start, {.com = volume_down}},
	{MOD, XK_o, start, {.com = volume_toggle}},
	{MOD | CONTROL, XK_l, start, {.com = i3lock}},
	/* Exit jwm */
	{MOD | SHIFT, XK_q, jwm_exit, {.i = 0}},
};

#endif
