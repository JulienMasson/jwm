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

#define BUTTONMASK      XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE
#define NET_WM_FIXED    0xffffffff      // Value in WM hint which means this window is fixed on all workspaces.
#define TWOBWM_NOWS     0xfffffffe      // This means we didn't get any window hint at all.
#define LENGTH(x)       (sizeof(x) / sizeof(*x))
#define MIN(X, Y)       ((X) < (Y) ? (X) : (Y))
#define CLEANMASK(mask) (mask & ~(numlockmask | XCB_MOD_MASK_LOCK))
#define CONTROL         XCB_MOD_MASK_CONTROL    /* Control key */
#define ALT             XCB_MOD_MASK_1          /* ALT key */
#define SHIFT           XCB_MOD_MASK_SHIFT      /* Shift key */

enum { TWOBWM_MOVE, TWOBWM_RESIZE };
enum { TWOBWM_FOCUS_NEXT, TWOBWM_FOCUS_PREVIOUS };
enum { BOTTOM_RIGHT, BOTTOM_LEFT, TOP_RIGHT, TOP_LEFT, MIDDLE };
enum { wm_delete_window, wm_change_state, NB_ATOMS };
enum { TWOBWM_MAXHALF_FOLD_HORIZONTAL, TWOBWM_MAXHALF_UNFOLD_HORIZONTAL, TWOBWM_MAXHALF_HORIZONTAL_TOP, TWOBWM_MAXHALF_HORIZONTAL_BOTTOM, MAXHALF_UNUSED, TWOBWM_MAXHALF_VERTICAL_RIGHT, TWOBWM_MAXHALF_VERTICAL_LEFT, TWOBWM_MAXHALF_UNFOLD_VERTICAL, TWOBWM_MAXHALF_FOLD_VERTICAL };
enum { TWOBWM_PREVIOUS_SCREEN, TWOBWM_NEXT_SCREEN };
