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
static const char *dmenu[] = { "dmenu_run", NULL };
static const char *urxvt[] = { "urxvt", NULL };
static const char *emacs[] = { "emacs", NULL };
static const char *volume_up[] = { "amixer", "set", "Master", "4%+", NULL };
static const char *volume_down[] = { "amixer", "set", "Master", "4%-", NULL };
static const char *volume_toggle[] = { "pactl", "set-sink-mute", "0", "toggle", NULL };
static const char *slock[] = { "slock", NULL };

/* Keyboard shorcut */
static key keys[] = {
	/* modifier           key            function           argument */
	/* Focus to next/previous window */
	{ MOD,		 XK_Right,  focusnext,	  { .i	 = FOCUS_NEXT		  } },
	{ MOD,		 XK_Left,   focusnext,	  { .i	 = FOCUS_PREVIOUS	  } },
	/* Vertically left/right */
	{ MOD | CONTROL, XK_Right,  maxhalf,	  { .i	 = MAXHALF_VERTICAL_RIGHT } },
	{ MOD | CONTROL, XK_Left,   maxhalf,	  { .i	 = MAXHALF_VERTICAL_LEFT  } },
	/* Next/Previous screen */
	{ MOD | SHIFT,	 XK_Right,  changescreen, { .i	 = PREVIOUS_SCREEN	  } },
	{ MOD | SHIFT,	 XK_Left,   changescreen, { .i	 = NEXT_SCREEN		  } },
	/* Kill a window */
	{ MOD | SHIFT,	 XK_c,	    deletewin,	  {} },
	/* Full screen window without borders */
	{ MOD,		 XK_f,	    maximize,	  { .i	 = FULLSCREEN_ONE_MONITOR } },
	{ MOD | CONTROL, XK_t,	    maximize,	  { .i	 = FULLSCREEN_ALL_MONITOR } },
	/* Hide / Raise windows */
	{ MOD,		 XK_n,	    hide,	  {} },
	{ MOD,		 XK_a,	    raise_all,	  {} },
	/* Programs */
	{ MOD,		 XK_d,	    start,	  { .com = dmenu			 } },
	{ MOD,		 XK_Return, start,	  { .com = urxvt			 } },
	{ MOD | SHIFT,	 XK_e,	    start,	  { .com = emacs			 } },
	{ MOD,		 XK_o,	    start,	  { .com = volume_up			 } },
	{ MOD,		 XK_i,	    start,	  { .com = volume_down			 } },
	{ MOD,		 XK_p,	    start,	  { .com = volume_toggle		 } },
	{ MOD | CONTROL, XK_l,	    start,	  { .com = slock			 } },
	/* Exit jwm */
	{ MOD | SHIFT,	 XK_q,	    jwm_exit,     { .i	 = 0				 } },
};

#endif
