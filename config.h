/*
 * This file is part of the jwm distribution (https://github.com/JulienMasson/jwm).
 * Copyright (c) 2017 Julien Masson.
 *
 * jwm is derived from 2bwm (https://github.com/venam/2bwm)
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

/* Super/Windows key */
#define MOD             XCB_MOD_MASK_4

/* 0) offsetx          1) offsety
 * 2) maxwidth         3) maxheight */
static const uint8_t offsets[] = { 0, 0, 0, 0 };

/* default position of the cursor:
 * correct values are:
 * TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT, MIDDLE
 * All these are relative to the current window. */
#define CURSOR_POSITION MIDDLE

/* Windows that won't have a border.
 * It uses substring comparison with what is found in the WM_NAME
 * attribute of the window. You can test this using `xprop WM_NAME`
 */
#define LOOK_INTO "WM_NAME"

/* Menus and Programs */
static const char *menucmd[] = { "dmenu_run", NULL };

/* Keyboard shorcut */
static key keys[] = {
	/* modifier           key            function           argument */
	/* Focus to next/previous window */
	{ MOD,		 XK_Right,  focusnext,	  { .i	 = TWOBWM_FOCUS_NEXT		 } },
	{ MOD,		 XK_Left,   focusnext,	  { .i	 = TWOBWM_FOCUS_PREVIOUS	 } },
	/* Vertically left/right */
	{ MOD | CONTROL, XK_Right,  maxhalf,	  { .i	 = TWOBWM_MAXHALF_VERTICAL_RIGHT } },
	{ MOD | CONTROL, XK_Left,   maxhalf,	  { .i	 = TWOBWM_MAXHALF_VERTICAL_LEFT	 } },
	/* Next/Previous screen */
	{ MOD | SHIFT,	 XK_Right,  changescreen, { .i	 = TWOBWM_PREVIOUS_SCREEN	 } },
	{ MOD | SHIFT,	 XK_Left,   changescreen, { .i	 = TWOBWM_NEXT_SCREEN		 } },
	/* Kill a window */
	{ MOD | SHIFT,	 XK_c,	    deletewin,	  {} },
	/* Full screen window without borders */
	{ MOD,		 XK_f,	    maximize,	  { .i	 = TWOBWM_FULLSCREEN		 } },
	/* Start programs */
	{ MOD,		 XK_d,	    start,	  { .com = menucmd			 } },
	{ MOD,		 XK_Return, start_urxvt,  {} },
	/* Exit or restart 2bwm */
	{ MOD | SHIFT,	 XK_q,	    twobwm_exit,  { .i	 = 0				 } },
};

/* Mouse buttons */
static Button buttons[] = {
	{ MOD, XCB_BUTTON_INDEX_1, mousemotion, { .i = TWOBWM_MOVE   }, false },
	{ MOD, XCB_BUTTON_INDEX_3, mousemotion, { .i = TWOBWM_RESIZE }, false },
};
