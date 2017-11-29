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

/* Types */
struct monitor {
	xcb_randr_output_t	id;
	char *			name;
	int16_t			y, x;           // X and Y.
	uint16_t		width, height;  // Width/Height in pixels.
	struct item *		item;           // Pointer to our place in output list.
};
typedef union {
	const char **	com;
	const int8_t	i;
} Arg;
typedef struct {
	unsigned int	mod;
	xcb_keysym_t	keysym;
	void (*func)(const Arg *);
	const Arg	arg;
} key;
typedef struct {
	unsigned int	mask, button;
	void (*func)(const Arg *);
	const Arg	arg;
	const bool	root_only;
} Button;
struct sizepos {
	int16_t		x, y;
	uint16_t	width, height;
};
struct client {                         // Everything we know about a window.
	xcb_drawable_t	id;             // ID of this window.
	bool		usercoord;      // X,Y was set by -geom.
	int16_t		x, y;           // X/Y coordinate.
	uint16_t	width, height;  // Width,Height in pixels.
	struct sizepos	origsize;       // Original size if we're currently maxed.
	uint16_t	max_width, max_height, min_width, min_height, width_inc, height_inc, base_width, base_height;
	bool		fixed, vertmaxed, hormaxed, maxed, verthor, iconic;
	struct monitor *monitor;        // The physical output this window is on.
	struct item *	winitem;        // Pointer to our place in global windows list.
	struct item *	wsitem;         // Pointer to our place in every workspace window list.
};
struct winconf {                        // Window configuration data.
	int16_t		x, y;
	uint16_t	width, height;
	uint8_t		stackmode;
	xcb_window_t	sibling;
};
