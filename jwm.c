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

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <bits/sigaction.h>
#include <xcb/randr.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_ewmh.h>
#include <X11/keysym.h>
#include "list.h"
#include "definitions.h"
#include "types.h"

/* Internal Constants */
/* Globals */
static void (*events[XCB_NO_OPERATION])(xcb_generic_event_t * e);
static unsigned int numlockmask = 0;
int sigcode;                            // Signal code. Non-zero if we've been interruped by a signal.
xcb_connection_t *conn;                 // Connection to X server.
xcb_ewmh_connection_t *ewmh;            // Ewmh Connection.
xcb_screen_t *screen;                   // Our current screen.
int randrbase;                          // Beginning of RANDR extension events.
struct client *focuswin;                // Current focus window.
static xcb_drawable_t top_win = 0;      // Window always on top.
static struct item *winlist = NULL;     // Global list of all client windows.
static struct item *monlist = NULL;     // List of all physical monitor outputs.
static struct item *wslist;
/* Global configuration.*/
static const char *atomnames[NB_ATOMS][1] = {
	{ "WM_DELETE_WINDOW" },
	{ "WM_CHANGE_STATE"  }
};

xcb_atom_t ATOM[NB_ATOMS];
/* Functions prototypes */
static void run(void);
static bool setup(int);
static void install_sig_handlers(void);
static void start(const Arg *);
static void mousemotion(const Arg *);
static void focusnext(const Arg *);
static void focusnext_helper(bool);
static void maxhalf(const Arg *);
static void changescreen(const Arg *);
static void grabkeys(void);
static void jwm_exit();
static bool setup_keyboard(void);
static int  setuprandr(void);
static void arrangewindows(void);
static void getrandr(void);
static void raise_current_window(void);
static void setunfocus(void);
static void maximize(const Arg *);
static void maximize_helper(struct client *, uint16_t, uint16_t, uint16_t, uint16_t);
static void hide();
static void clientmessage(xcb_generic_event_t *);
static void deletewin();
static void check_name(struct client *);
static void addtoclientlist(const xcb_drawable_t);
static void configurerequest(xcb_generic_event_t *);
static void buttonpress(xcb_generic_event_t *);
static void unmapnotify(xcb_generic_event_t *);
static void destroynotify(xcb_generic_event_t *);
static void circulaterequest(xcb_generic_event_t *);
static void newwin(xcb_generic_event_t *);
static void handle_keypress(xcb_generic_event_t *);
static xcb_cursor_t Create_Font_Cursor(xcb_connection_t *, uint16_t);
static xcb_keycode_t *xcb_get_keycodes(xcb_keysym_t);
static xcb_screen_t *xcb_screen_of_display(xcb_connection_t *, int);
static struct client *setupwin(xcb_window_t);
static struct client create_back_win(void);
static void cleanup(void);
static void grabbuttons(struct client *);
static void forgetclient(struct client *);
static void forgetwin(xcb_window_t);
static void get_borders_all_mons(uint16_t *, uint16_t *);
static void fitonscreen(struct client *);
static void getoutputs(xcb_randr_output_t *, const int, xcb_timestamp_t);
static void arrbymon(struct monitor *);
static struct monitor *findmonitor(xcb_randr_output_t);
static struct monitor *findclones(xcb_randr_output_t, const int16_t, const int16_t);
static struct monitor *findmonbycoord(const int16_t, const int16_t);
static void delmonitor(struct monitor *);
static struct monitor *addmonitor(xcb_randr_output_t, char *, const int16_t, const int16_t, const uint16_t, const uint16_t);
static void raisewindow(xcb_drawable_t);
static void raise_all(void);
static void movelim(struct client *);
static void movewindow(xcb_drawable_t, const int16_t, const int16_t);
static struct client *findclient(const xcb_drawable_t *);
static void setfocus(struct client *);
static void resizelim(struct client *);
static void resize(xcb_drawable_t, const uint16_t, const uint16_t);
static void moveresize(xcb_drawable_t, const uint16_t, const uint16_t, const uint16_t, const uint16_t);
static void mousemove(const int16_t, const int16_t);
static void mouseresize(struct client *, const int16_t, const int16_t);
static void unmax(struct client *);
static bool getpointer(const xcb_drawable_t *, int16_t *, int16_t *);
static bool getgeom(const xcb_drawable_t *, int16_t *, int16_t *, uint16_t *, uint16_t *);
static void configwin(xcb_window_t, uint16_t, const struct winconf *);
static void sigcatch(const int);
static void ewmh_init(void);
static xcb_atom_t getatom(const char *);
static void getmonsize(int16_t *, int16_t *, uint16_t *, uint16_t *, const struct client *);
#include "config.h"

/* Function bodies */
void
delmonitor(struct monitor *mon)
{
	freeitem(&monlist, NULL, mon->item);
}

void
raise_current_window(void)
{
	raisewindow(focuswin->id);
}

void
focusnext(const Arg *arg)
{
	focusnext_helper(arg->i > 0);
}

void
jwm_exit()
{
	exit(EXIT_SUCCESS);
}

void
saveorigsize(struct client *client)
{
	client->origsize.x = client->x;
	client->origsize.y = client->y;
	client->origsize.width = client->width;
	client->origsize.height = client->height;
}

void
centerpointer(xcb_drawable_t win, struct client *cl)
{
	int16_t cur_x, cur_y;

	cur_x = cur_y = 0;

	switch (CURSOR_POSITION) {
	case BOTTOM_RIGHT:
		cur_x += cl->width;
	case BOTTOM_LEFT:
		cur_y += cl->height; break;
	case TOP_RIGHT:
		cur_x += cl->width;
	case TOP_LEFT:
		break;
	default:
		cur_x = cl->width / 2;
		cur_y = cl->height / 2;
	}

	xcb_warp_pointer(conn, XCB_NONE, win, 0, 0, 0, 0, cur_x, cur_y);
}

void
updateclientlist(void)
{
	uint32_t len, i;
	xcb_window_t *children;
	struct client *cl;

	/* can only be called after the first window has been spawn */
	xcb_query_tree_reply_t *reply = xcb_query_tree_reply(conn,
							     xcb_query_tree(conn, screen->root), 0);

	xcb_delete_property(conn, screen->root, ewmh->_NET_CLIENT_LIST);
	xcb_delete_property(conn, screen->root, ewmh->_NET_CLIENT_LIST_STACKING);

	if (reply == NULL) {
		addtoclientlist(0);
		return;
	}

	len = xcb_query_tree_children_length(reply);
	children = xcb_query_tree_children(reply);

	for (i = 0; i < len; i++) {
		cl = findclient(&children[i]);
		if (cl != NULL)
			addtoclientlist(cl->id);
	}

	free(reply);
}

/* get screen of display */
xcb_screen_t *
xcb_screen_of_display(xcb_connection_t *con, int screen)
{
	xcb_screen_iterator_t iter;

	iter = xcb_setup_roots_iterator(xcb_get_setup(con));
	for (; iter.rem; --screen, xcb_screen_next(&iter))
		if (screen == 0)
			return iter.data;

	return NULL;
}

/* Set keyboard focus to follow mouse pointer. Then exit. We don't need to
 * bother mapping all windows we know about. They should all be in the X
 * server's Save Set and should be mapped automagically. */
void
cleanup(void)
{
	xcb_set_input_focus(conn, XCB_NONE, XCB_INPUT_FOCUS_POINTER_ROOT,
			    XCB_CURRENT_TIME);
	/* delallitems(wslist, NULL); */
	xcb_ewmh_connection_wipe(ewmh);
	xcb_flush(conn);

	if (ewmh != NULL)
		free(ewmh);

	xcb_disconnect(conn);
}

/* Rearrange windows to fit new screen size. */
void
arrangewindows(void)
{
	struct client *client;
	struct item *item;

	/* Go through all windows and resize them appropriately to
	 * fit the screen. */

	for (item = winlist; item != NULL; item = item->next) {
		client = item->data;
		fitonscreen(client);
	}
}

void
check_name(struct client *client)
{
	xcb_get_property_reply_t *reply;
	unsigned int reply_len;
	char *wm_name_window;

	if (NULL == client)
		return;

	reply = xcb_get_property_reply(conn, xcb_get_property(conn, false,
							      client->id, getatom(LOOK_INTO),
							      XCB_GET_PROPERTY_TYPE_ANY, 0, 60), NULL
				       );

	if (reply == NULL || xcb_get_property_value_length(reply) == 0) {
		if (NULL != reply)
			free(reply);
		return;
	}

	reply_len = xcb_get_property_value_length(reply);
	wm_name_window = malloc(sizeof(char *) * (reply_len + 1));;
	memcpy(wm_name_window, xcb_get_property_value(reply), reply_len);
	wm_name_window[reply_len] = '\0';

	if (NULL != reply)
		free(reply);

	free(wm_name_window);
}

static void addtoclientlist(const xcb_drawable_t id)
{
	xcb_change_property(conn, XCB_PROP_MODE_APPEND, screen->root, ewmh->_NET_CLIENT_LIST, XCB_ATOM_WINDOW, 32, 1, &id);
	xcb_change_property(conn, XCB_PROP_MODE_APPEND, screen->root, ewmh->_NET_CLIENT_LIST_STACKING, XCB_ATOM_WINDOW, 32, 1, &id);
}

/* Forget everything about client client. */
void
forgetclient(struct client *client)
{
	if (NULL == client)
		return;

	if (client->id == top_win)
		top_win = 0;

	/* Delete client from the workspace lists it belongs to.
	 * (can be on several) */
	if (NULL != client->wsitem) {
		delitem(&wslist, client->wsitem);
		client->wsitem = NULL;
	}

	/* Remove from global window list. */
	freeitem(&winlist, NULL, client->winitem);
}

/* Forget everything about a client with client->id win. */
void
forgetwin(xcb_window_t win)
{
	struct client *client;
	struct item *item;

	for (item = winlist; item != NULL; item = item->next) {
		/* Find this window in the global window list. */
		client = item->data;
		if (win == client->id) {
			/* Forget it and free allocated data, it might
			 * already be freed by handling an UnmapNotify. */
			forgetclient(client);
			return;
		}
	}
}

void
get_borders_all_mons(uint16_t *border_x, uint16_t *border_y)
{
	struct monitor *mon;
	struct item *item;
	uint16_t x=0, width=0, height=0;

	for (item = monlist; item != NULL; item = item->next) {
		mon = item->data;
		if (mon->x > x) {
			x = mon->x;
			width = mon->width;
		}
		if (mon->height > height)
			height = mon->height;
	}

	/* assign values */
	*border_x = x + width;
	*border_y = height;
}

void
getmonsize(int16_t *mon_x, int16_t *mon_y, uint16_t *mon_width, uint16_t *mon_height,
	   const struct client *client)
{
	if (NULL == client || NULL == client->monitor) {
		/* Window isn't attached to any monitor, so we use
		 * the root window size. */
		*mon_x = *mon_y = 0;
		*mon_width = screen->width_in_pixels;
		*mon_height = screen->height_in_pixels;
	} else {
		*mon_x = client->monitor->x;
		*mon_y = client->monitor->y;
		*mon_width = client->monitor->width;
		*mon_height = client->monitor->height;
	}
}

void
maximize_helper(struct client *client, uint16_t mon_x, uint16_t mon_y,
		uint16_t mon_width, uint16_t mon_height)
{
	uint32_t values[4];

	values[0] = 0;
	saveorigsize(client);
	xcb_configure_window(conn, client->id, XCB_CONFIG_WINDOW_BORDER_WIDTH,
			     values);

	client->x = mon_x;
	client->y = mon_y;
	client->width = mon_width;
	client->height = mon_height;

	moveresize(client->id, client->x, client->y, client->width,
		   client->height);
	client->maxed = true;
}

/* Fit client on physical screen, moving and resizing as necessary. */
void
fitonscreen(struct client *client)
{
	int16_t mon_x, mon_y;
	uint16_t mon_width, mon_height;
	bool willmove, willresize;

	willmove = willresize = client->vertmaxed = client->hormaxed = false;

	getmonsize(&mon_x, &mon_y, &mon_width, &mon_height, client);

	if (client->maxed) {
		client->maxed = false;
	} else {
		/* not maxed but look as if it was maxed, then make it maxed */
		if (client->width == mon_width && client->height == mon_height) {
			willresize = true;
		} else {
			getmonsize(&mon_x, &mon_y, &mon_width, &mon_height,
				   client);
			if (client->width == mon_width && client->height
			    == mon_height)
				willresize = true;
		}
		if (willresize) {
			client->x = mon_x;
			client->y = mon_y;
			maximize_helper(client, mon_x, mon_y, mon_width,
					mon_height);
			return;
		} else {
			getmonsize(&mon_x, &mon_y, &mon_width,
				   &mon_height, client);
		}
	}

	if (client->x > mon_x + mon_width || client->y > mon_y + mon_height
	    || client->x < mon_x || client->y < mon_y) {
		willmove = true;
		/* Is it outside the physical monitor? */
		if (client->x > mon_x + mon_width)
			client->x = mon_x + mon_width;

		if (client->y > mon_y + mon_height)
			client->y = mon_y + mon_height - client->height;

		if (client->x < mon_x)
			client->x = mon_x;
		if (client->y < mon_y)
			client->y = mon_y;
	}

	/* Is it smaller than it wants to  be? */
	if (0 != client->min_height && client->height < client->min_height) {
		client->height = client->min_height;

		willresize = true;
	}

	if (0 != client->min_width && client->width < client->min_width) {
		client->width = client->min_width;
		willresize = true;
	}

	/* If the window is larger than our screen, just place it in
	 * the corner and resize. */
	if (client->width > mon_width) {
		client->x = mon_x;
		client->width = mon_width;
		willmove = willresize = true;
	} else
	if (client->x + client->width > mon_x + mon_width) {
		client->x = mon_x + mon_width;
		willmove = true;
	}
	if (client->height > mon_height) {
		client->y = mon_y;
		client->height = mon_height;
		willmove = willresize = true;
	} else
	if (client->y + client->height > mon_y + mon_height) {
		client->y = mon_y + mon_height - client->height;
		willmove = true;
	}

	if (willmove)
		movewindow(client->id, client->x, client->y);

	if (willresize)
		resize(client->id, client->width, client->height);
}


/* Set position, geometry and attributes of a new window and show it on
 * the screen.*/
void
newwin(xcb_generic_event_t *ev)
{
	xcb_map_request_event_t *e = (xcb_map_request_event_t *)ev;
	struct client *client;
	long data[] = {
		XCB_ICCCM_WM_STATE_NORMAL,
		XCB_NONE
	};

	/* The window is trying to map itself on the current workspace,
	 * but since it's unmapped it probably belongs on another workspace.*/
	if (NULL != findclient(&e->window))
		return;

	client = setupwin(e->window);

	if (NULL == client)
		return;

	/* Add this window to the current workspace. */
	struct item *item = additem(&wslist);
	client->wsitem = item;
	item->data = client;

	/* If we don't have specific coord map it where the pointer is.*/
	if (!client->usercoord) {
		if (!getpointer(&screen->root, &client->x, &client->y))
			client->x = client->y = 0;

		client->x -= client->width / 2;    client->y -= client->height / 2;
		movewindow(client->id, client->x, client->y);
	}

	/* Find the physical output this window will be on if RANDR is active */
	if (-1 != randrbase) {
		client->monitor = findmonbycoord(client->x, client->y);
		if (NULL == client->monitor && NULL != monlist)
			/* Window coordinates are outside all physical monitors.
			 * Choose the first screen.*/
			client->monitor = monlist->data;
	}

	fitonscreen(client);

	/* Show window on screen. */
	xcb_map_window(conn, client->id);
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, client->id,
			    ewmh->_NET_WM_STATE, ewmh->_NET_WM_STATE, 32, 2, data);

	centerpointer(e->window, client);
	updateclientlist();
}

/* Set border colour, width and event mask for window. */
struct client *
setupwin(xcb_window_t win)
{
	unsigned int i;
	uint32_t values[2];
	xcb_atom_t a;
	xcb_size_hints_t hints;
	xcb_ewmh_get_atoms_reply_t win_type;
	struct item *item;
	struct client *client;

	if (xcb_ewmh_get_wm_window_type_reply(ewmh,
					      xcb_ewmh_get_wm_window_type(ewmh, win),
					      &win_type, NULL) == 1) {
		for (i = 0; i < win_type.atoms_len; i++) {
			a = win_type.atoms[i];
			if (a == ewmh->_NET_WM_WINDOW_TYPE_TOOLBAR || a
			    == ewmh->_NET_WM_WINDOW_TYPE_DOCK || a
			    == ewmh->_NET_WM_WINDOW_TYPE_DESKTOP) {
				xcb_ewmh_get_atoms_reply_wipe(&win_type);
				xcb_map_window(conn, win);
				return NULL;
			}
		}
	}

	values[0] = XCB_EVENT_MASK_ENTER_WINDOW;
	/* xcb_change_window_attributes(conn, win, XCB_CW_BACK_PIXEL, */
	/*              &conf.empty_col); */
	xcb_change_window_attributes_checked(conn, win, XCB_CW_EVENT_MASK,
					     values);

	/* Add this window to the X Save Set. */
	xcb_change_save_set(conn, XCB_SET_MODE_INSERT, win);

	/* Remember window and store a few things about it. */
	item = additem(&winlist);

	if (NULL == item)
		return NULL;

	client = malloc(sizeof(struct client));

	if (NULL == client)
		return NULL;

	item->data = client;
	client->id = win;
	client->x = client->y = client->width = client->height
							= client->min_width = client->min_height = client->base_width
													   = client->base_height = 0;

	client->max_width = screen->width_in_pixels;
	client->max_height = screen->height_in_pixels;
	client->width_inc = client->height_inc = 1;
	client->usercoord = client->vertmaxed = client->hormaxed
							= client->maxed = client->fixed
										  = client->iconic = false;

	client->monitor = NULL;
	client->winitem = item;

	/* Initialize workspace pointers. */
	client->wsitem = NULL;

	/* Get window geometry. */
	getgeom(&client->id, &client->x, &client->y, &client->width,
		&client->height);

	/* Get the window's incremental size step, if any.*/
	xcb_icccm_get_wm_normal_hints_reply(conn,
					    xcb_icccm_get_wm_normal_hints_unchecked(conn, win),
					    &hints, NULL
					    );

	/* The user specified the position coordinates.
	 * Remember that so we can use geometry later. */
	if (hints.flags & XCB_ICCCM_SIZE_HINT_US_POSITION)
		client->usercoord = true;

	if (hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE) {
		client->min_width = hints.min_width;
		client->min_height = hints.min_height;
	}

	if (hints.flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE) {
		client->max_width = hints.max_width;
		client->max_height = hints.max_height;
	}

	if (hints.flags & XCB_ICCCM_SIZE_HINT_P_RESIZE_INC) {
		client->width_inc = hints.width_inc;
		client->height_inc = hints.height_inc;
	}

	if (hints.flags & XCB_ICCCM_SIZE_HINT_BASE_SIZE) {
		client->base_width = hints.base_width;
		client->base_height = hints.base_height;
	}

	check_name(client);
	return client;
}

/* wrapper to get xcb keycodes from keysymbol */
xcb_keycode_t *
xcb_get_keycodes(xcb_keysym_t keysym)
{
	xcb_key_symbols_t *keysyms;
	xcb_keycode_t *keycode;

	if (!(keysyms = xcb_key_symbols_alloc(conn)))
		return NULL;

	keycode = xcb_key_symbols_get_keycode(keysyms, keysym);
	xcb_key_symbols_free(keysyms);

	return keycode;
}

/* the wm should listen to key presses */
void
grabkeys(void)
{
	xcb_keycode_t *keycode;
	int i, k, m;
	unsigned int modifiers[] = {
		0,
		XCB_MOD_MASK_LOCK,
		numlockmask,
		numlockmask | XCB_MOD_MASK_LOCK
	};

	xcb_ungrab_key(conn, XCB_GRAB_ANY, screen->root, XCB_MOD_MASK_ANY);

	for (i = 0; i < LENGTH(keys); i++) {
		keycode = xcb_get_keycodes(keys[i].keysym);

		for (k = 0; keycode[k] != XCB_NO_SYMBOL; k++)
			for (m = 0; m < LENGTH(modifiers); m++)
				xcb_grab_key(conn, 1, screen->root, keys[i].mod
					     | modifiers[m], keycode[k],
					     XCB_GRAB_MODE_ASYNC,       //pointer mode
					     XCB_GRAB_MODE_ASYNC        //keyboard mode
					     );
		free(keycode);                                          // allocated in xcb_get_keycodes()
	}
}

bool
setup_keyboard(void)
{
	xcb_get_modifier_mapping_reply_t *reply;
	xcb_keycode_t *modmap, *numlock;
	unsigned int i, j, n;

	reply = xcb_get_modifier_mapping_reply(conn,
					       xcb_get_modifier_mapping_unchecked(conn), NULL);

	if (!reply)
		return false;

	modmap = xcb_get_modifier_mapping_keycodes(reply);

	if (!modmap)
		return false;

	numlock = xcb_get_keycodes(XK_Num_Lock);

	for (i = 4; i < 8; i++) {
		for (j = 0; j < reply->keycodes_per_modifier; j++) {
			xcb_keycode_t keycode = modmap[i
						       * reply->keycodes_per_modifier + j];

			if (keycode == XCB_NO_SYMBOL)
				continue;

			if (numlock != NULL) {
				for (n = 0; numlock[n] != XCB_NO_SYMBOL; n++)
					if (numlock[n] == keycode) {
						numlockmask = 1 << i;
						break;
					}
			}
		}
	}

	free(reply);
	free(numlock);

	return true;
}

/* Set up RANDR extension. Get the extension base and subscribe to events */
int
setuprandr(void)
{
	int base;
	const xcb_query_extension_reply_t *extension
		= xcb_get_extension_data(conn, &xcb_randr_id);

	if (!extension->present)
		return -1;
	else
		getrandr();

	base = extension->first_event;
	xcb_randr_select_input(conn, screen->root,
			       XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE
			       | XCB_RANDR_NOTIFY_MASK_OUTPUT_CHANGE
			       | XCB_RANDR_NOTIFY_MASK_CRTC_CHANGE
			       | XCB_RANDR_NOTIFY_MASK_OUTPUT_PROPERTY
			       );

	return base;
}

/* Get RANDR resources and figure out how many outputs there are. */
void
getrandr(void)
{
	int len;
	xcb_randr_get_screen_resources_current_cookie_t rcookie
		= xcb_randr_get_screen_resources_current(conn, screen->root);
	xcb_randr_get_screen_resources_current_reply_t *res
		= xcb_randr_get_screen_resources_current_reply(conn, rcookie,
							       NULL);

	if (NULL == res)
		return;

	xcb_timestamp_t timestamp = res->config_timestamp;
	len = xcb_randr_get_screen_resources_current_outputs_length(res);
	xcb_randr_output_t *outputs
		= xcb_randr_get_screen_resources_current_outputs(res);

	/* Request information for all outputs. */
	getoutputs(outputs, len, timestamp);
	free(res);
}

/* Walk through all the RANDR outputs (number of outputs == len) there */
void
getoutputs(xcb_randr_output_t *outputs, const int len,
	   xcb_timestamp_t timestamp)
{
	int i;
	int name_len;
	char *name;

	/* was at time timestamp. */
	xcb_randr_get_crtc_info_cookie_t icookie;
	xcb_randr_get_crtc_info_reply_t *crtc = NULL;
	xcb_randr_get_output_info_reply_t *output;
	struct monitor *mon, *clonemon;
	struct item *item;
	xcb_randr_get_output_info_cookie_t ocookie[len];

	for (i = 0; i < len; i++)
		ocookie[i] = xcb_randr_get_output_info(conn, outputs[i],
						       timestamp);

	/* Loop through all outputs. */
	for (i = 0; i < len; i++) {
		if ((output = xcb_randr_get_output_info_reply(conn, ocookie[i],
							      NULL)) == NULL)
			continue;

		name_len = MIN(16, xcb_randr_get_output_info_name_length(output));
		name = malloc(name_len + 1);

		snprintf(name, name_len + 1, "%.*s", name_len,
			 xcb_randr_get_output_info_name(output));

		if (XCB_NONE != output->crtc) {
			icookie = xcb_randr_get_crtc_info(conn, output->crtc,
							  timestamp);
			crtc = xcb_randr_get_crtc_info_reply(conn, icookie, NULL);

			if (NULL == crtc)
				return;

			/* Check if it's a clone. */
			clonemon = findclones(outputs[i], crtc->x, crtc->y);

			if (NULL != clonemon)
				continue;

			/* Do we know this monitor already? */
			if (NULL == (mon = findmonitor(outputs[i]))) {
				addmonitor(outputs[i], name, crtc->x, crtc->y,
					   crtc->width, crtc->height);
			} else
			/* We know this monitor. Update information.
			 * If it's smaller than before, rearrange windows. */
			if (crtc->x != mon->x || crtc->y != mon->y || crtc->width
			    != mon->width || crtc->height
			    != mon->height) {
				if (crtc->x != mon->x)
					mon->x = crtc->x;
				if (crtc->y != mon->y)
					mon->y = crtc->y;
				if (crtc->width != mon->width)
					mon->width = crtc->width;
				if (crtc->height != mon->height)
					mon->height = crtc->height;

				arrbymon(mon);
			}
			free(crtc);
		} else {
			/* Check if it was used before. If it was, do something. */
			if ((mon = findmonitor(outputs[i]))) {
				struct client *client;
				for (item = winlist; item != NULL; item = item->next) {
					/* Check all windows on this monitor
					 * and move them to the next or to the
					 * first monitor if there is no next. */
					client = item->data;

					if (client->monitor == mon) {
						if (NULL == client->monitor->item->next) {
							if (NULL == monlist)
								client->monitor = NULL;
							else
								client->monitor = monlist->data;
						} else {
							client->monitor = client->monitor->item->next->data;
						}
						fitonscreen(client);
					}
				}

				/* It's not active anymore. Forget about it. */
				delmonitor(mon);
			}
		}
		if (NULL != output)
			free(output);

		free(name);
	} /* for */
}

void
arrbymon(struct monitor *monitor)
{
	struct client *client;
	struct item *item;

	for (item = winlist; item != NULL; item = item->next) {
		client = item->data;

		if (client->monitor == monitor)
			fitonscreen(client);
	}
}

struct monitor *
findmonitor(xcb_randr_output_t id)
{
	struct monitor *mon;
	struct item *item;

	for (item = monlist; item != NULL; item = item->next) {
		mon = item->data;

		if (id == mon->id)
			return mon;
	}

	return NULL;
}

struct monitor *
findclones(xcb_randr_output_t id, const int16_t x, const int16_t y)
{
	struct monitor *clonemon;
	struct item *item;

	for (item = monlist; item != NULL; item = item->next) {
		clonemon = item->data;

		/* Check for same position. */
		if (id != clonemon->id && clonemon->x == x && clonemon->y == y)
			return clonemon;
	}

	return NULL;
}

struct monitor *
findmonbycoord(const int16_t x, const int16_t y)
{
	struct monitor *mon;
	struct item *item;

	for (item = monlist; item != NULL; item = item->next) {
		mon = item->data;

		if (x >= mon->x && x <= mon->x + mon->width && y >= mon->y && y
		    <= mon->y + mon->height)
			return mon;
	}

	return NULL;
}

struct monitor *
addmonitor(xcb_randr_output_t id, char *name, const int16_t x, const int16_t y,
	   const uint16_t width, const uint16_t height)
{
	struct item *item;
	struct monitor *mon = malloc(sizeof(struct monitor));

	if (NULL == (item = additem(&monlist)))
		return NULL;

	if (NULL == mon)
		return NULL;

	item->data = mon;
	mon->id = id;
	mon->name = name;
	mon->item = item;
	mon->x = x;
	mon->y = y;
	mon->width = width;
	mon->height = height;

	return mon;
}

/* Raise window win to top of stack. */
void
raisewindow(xcb_drawable_t win)
{
	uint32_t values[] = { XCB_STACK_MODE_ABOVE };

	if (screen->root == win || 0 == win)
		return;

	xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_STACK_MODE, values);
	xcb_flush(conn);
}

void
raise_all(void)
{
	struct client *cl = NULL;

	if (wslist && wslist->data)
		cl = wslist->data;
	else
		return;

	do {
		if (cl->iconic == true) {
			cl->iconic = false;
			xcb_map_window(conn, cl->id);
			xcb_flush(conn);
		}
	} while (cl->wsitem->next && (cl = cl->wsitem->next->data));
}

/* Move the window to the corresponding screen */
void
movelim(struct client *client)
{
	struct monitor *mon;
	struct item *item;
	int16_t mon_y, mon_x;
	int16_t middle_x;
	uint16_t border_x, border_y;

	/* select right monitor with the middle of the client window */
	for (item = monlist; item != NULL; item = item->next) {
		mon = item->data;
		middle_x = client->x + (client->width / 2);

		if ((middle_x > mon->x) && (middle_x < (mon->x + mon->width))) {
			/* change monitor if we move to an another monitor */
			if (client->monitor != mon)
				client->monitor = mon;
		}
	}

	get_borders_all_mons(&border_x, &border_y);

	/* Is it outside the physical monitor or close to the side? */
	if (client->y < mon_y)
		client->y = mon_y;
	else if (client->y + client->height > border_y)
		client->y = border_y - client->height;

	if (client->x < mon_x)
		client->x = mon_x;
	else if (client->x + client->width > border_x)
		client->x = border_x - client->width;

	movewindow(client->id, client->x, client->y);
}

void
movewindow(xcb_drawable_t win, const int16_t x, const int16_t y)
{                                    // Move window win to root coordinates x,y.
	uint32_t values[2] = { x, y };

	if (screen->root == win || 0 == win)
		return;

	xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_X
			     | XCB_CONFIG_WINDOW_Y, values);

	xcb_flush(conn);
}

/* Change focus to next in window ring. */
void
focusnext_helper(bool arg)
{
	struct client *cl = NULL;

	/* If we currently have no focus focus first in list. */
	if (NULL == focuswin || NULL == focuswin->wsitem) {
		cl = wslist->data;
		while (cl->iconic == true && cl->wsitem->next != NULL)
			cl = cl->wsitem->next->data;
	} else {
		if (arg == FOCUS_NEXT) {
			if (NULL == focuswin->wsitem->prev) {
				/* We were at the head of list.
				 * Focusing on last window in list unless
				 * we were already there.*/
				cl = wslist->data;

				/* Go to the end of the list */
				while (cl->wsitem->next != NULL)
					cl = cl->wsitem->next->data;
				/* walk backward until we find
				 * a window that isn't iconic */
				while (cl->iconic == true)
					cl = cl->wsitem->prev->data;
			} else
			if (focuswin != wslist->data) {
				cl = focuswin->wsitem->prev->data;
				while (cl->iconic == true
				       && cl->wsitem->prev
				       != NULL)
					cl = cl->wsitem->prev->data;
				/* move to the head an didn't find a
				 * window to focus so move to the end
				 * starting from the focused win */
				if (cl->iconic == true) {
					cl = focuswin;
					/* Go to the end of the list */
					while (cl->wsitem->next
					       != NULL)
						cl = cl->wsitem->next->data;
					while (cl->iconic == true)
						cl = cl->wsitem->prev->data;
				}
			}
		} else {
			/* We were at the tail of list.
			 * Focusing on last window in list unless we
			 * were already there.*/
			if (NULL == focuswin->wsitem->next) {
				/* We were at the end of list.
				 * Focusing on first window in list unless we
				 * were already there. */
				cl = wslist->data;
				while (cl->iconic && cl->wsitem->next
				       != NULL)
					cl = cl->wsitem->next->data;
			} else {
				cl = focuswin->wsitem->next->data;
				while (cl->iconic == true
				       && cl->wsitem->next
				       != NULL)
					cl = cl->wsitem->next->data;
				/* we reached the end of the list without a
				 * new win to focus, so reloop from the head */
				if (cl->iconic == true) {
					cl = wslist->data;
					while (cl->iconic
					       && cl->wsitem->next
					       != NULL)
						cl = cl->wsitem->next->data;
				}
			}
		}
	}
	/* if NULL focuswin */
	if (NULL != cl && focuswin != cl && cl->iconic == false) {
		raisewindow(cl->id);
		centerpointer(cl->id, cl);
		setfocus(cl);
	}
}
/* Mark window win as unfocused. */
void setunfocus(void)
{
//    xcb_set_input_focus(conn, XCB_NONE, XCB_INPUT_FOCUS_NONE,XCB_CURRENT_TIME);
	if (NULL == focuswin || focuswin->id == screen->root)
		return;
}

/* Find client with client->id win in global window list or NULL. */
struct client *
findclient(const xcb_drawable_t *win)
{
	struct client *client;
	struct item *item;

	for (item = winlist; item != NULL; item = item->next) {
		client = item->data;

		if (*win == client->id)
			return client;
	}

	return NULL;
}

void
setfocus(struct client *client)// Set focus on window client.
{
	long data[] = { XCB_ICCCM_WM_STATE_NORMAL, XCB_NONE };

	/* If client is NULL, we focus on whatever the pointer is on.
	 * This is a pathological case, but it will make the poor user able
	 * to focus on windows anyway, even though this windowmanager might
	 * be buggy. */
	if (NULL == client) {
		focuswin = NULL;
		xcb_set_input_focus(conn, XCB_NONE,
				    XCB_INPUT_FOCUS_POINTER_ROOT, XCB_CURRENT_TIME);
		xcb_window_t not_win = 0;
		xcb_change_property(conn, XCB_PROP_MODE_REPLACE, screen->root,
				    ewmh->_NET_ACTIVE_WINDOW, XCB_ATOM_WINDOW, 32, 1,
				    &not_win);

		xcb_flush(conn);
		return;
	}

	/* Don't bother focusing on the root window or on the same window
	 * that already has focus. */
	if (client->id == screen->root)
		return;

	if (NULL != focuswin)
		setunfocus(); /* Unset last focus. */

	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, client->id,
			    ewmh->_NET_WM_STATE, ewmh->_NET_WM_STATE, 32, 2, data);
	xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT, client->id,
			    XCB_CURRENT_TIME); /* Set new input focus. */
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, screen->root,
			    ewmh->_NET_ACTIVE_WINDOW, XCB_ATOM_WINDOW, 32, 1, &client->id);

	/* Remember the new window as the current focused window. */
	focuswin = client;

	grabbuttons(client);
}

void
start(const Arg *arg)
{
	if (fork())
		return;

	if (conn)
		close(screen->root);

	setsid();
	execvp((char *)arg->com[0], (char **)arg->com);
}

/* Resize with limit. */
void
resizelim(struct client *client)
{
	uint16_t border_x, border_y;

	get_borders_all_mons(&border_x, &border_y);

	/* Minimum height */
	if (0 != client->min_height && client->height < client->min_height)
		client->height = client->min_height;

	/* Minimum width */
	if (0 != client->min_width && client->width < client->min_width)
		client->width = client->min_width;

	/* Maximum x */
	if (client->x + client->width > border_x)
		client->width = border_x - client->x;

	/* Maximum y */
	if (client->y + client->height > border_y)
		client->height = border_y - client->y;

	resize(client->id, client->width, client->height);
}

void
moveresize(xcb_drawable_t win, const uint16_t x, const uint16_t y,
	   const uint16_t width, const uint16_t height)
{
	uint32_t values[4] = { x, y, width, height };

	if (screen->root == win || 0 == win)
		return;

	xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_X
			     | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH
			     | XCB_CONFIG_WINDOW_HEIGHT, values);

	xcb_flush(conn);
}

/* Resize window win to width,height. */
void
resize(xcb_drawable_t win, const uint16_t width, const uint16_t height)
{
	uint32_t values[2] = { width, height };

	if (screen->root == win || 0 == win)
		return;

	xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_WIDTH
			     | XCB_CONFIG_WINDOW_HEIGHT, values);

	xcb_flush(conn);
}

/* Move window win as a result of pointer motion to coordinates rel_x,rel_y. */
void
mousemove(const int16_t rel_x, const int16_t rel_y)
{
	if (focuswin == NULL)
		return;

	focuswin->x = rel_x;
	focuswin->y = rel_y;

	movelim(focuswin);
}

void
mouseresize(struct client *client, const int16_t rel_x, const int16_t rel_y)
{
	if (focuswin->id == screen->root || focuswin->maxed)
		return;

	client->width = abs(rel_x);
	client->height = abs(rel_y);

	resizelim(client);
	client->vertmaxed = false;
	client->hormaxed = false;
}

void
unmax(struct client *client)
{
	if (NULL == client)
		return;

	client->x = client->origsize.x;
	client->y = client->origsize.y;
	client->width = client->origsize.width;
	client->height = client->origsize.height;

	client->maxed = client->hormaxed = 0;
	moveresize(client->id, client->x, client->y,
		   client->width, client->height);

	centerpointer(client->id, client);
}

void
maximize(const Arg *arg)
{
	int16_t mon_x, mon_y;
	uint16_t mon_width, mon_height;

	if (NULL == focuswin)
		return;

	/* Check if maximized already. If so, revert to stored geometry. */
	if (focuswin->maxed) {
		unmax(focuswin);
		focuswin->maxed = false;
		return;
	}

	if (arg->i == FULLSCREEN_ONE_MONITOR) {
		getmonsize(&mon_x, &mon_y, &mon_width, &mon_height, focuswin);
	} else if (arg->i == FULLSCREEN_ALL_MONITOR) {
		mon_x = 0;
		mon_y = 0;
		get_borders_all_mons(&mon_width, &mon_height);
	}

	maximize_helper(focuswin, mon_x, mon_y, mon_width, mon_height);
	raise_current_window();
	xcb_flush(conn);
}

void
maxhalf(const Arg *arg)
{
	int16_t mon_x, mon_y;
	uint16_t mon_width, mon_height;

	if (NULL == focuswin || focuswin->maxed)
		return;

	getmonsize(&mon_x, &mon_y, &mon_width, &mon_height, focuswin);

	focuswin->y = mon_y;
	focuswin->height = mon_height;
	focuswin->width = ((float)(mon_width) / 2);

	if (arg->i == MAXHALF_VERTICAL_LEFT)
		focuswin->x = mon_x;
	else
		focuswin->x = mon_x + mon_width - focuswin->width;

	moveresize(focuswin->id, focuswin->x, focuswin->y,
		   focuswin->width, focuswin->height);

	focuswin->verthor = true;
	raise_current_window();
	fitonscreen(focuswin);
	centerpointer(focuswin->id, focuswin);
}

void
hide(void)
{
	if (focuswin == NULL)
		return;

	long data[] = {
		XCB_ICCCM_WM_STATE_ICONIC,
		ewmh->_NET_WM_STATE_HIDDEN,
		XCB_NONE
	};

	/* Unmap window and declare iconic. Unmapping will generate an
	 * UnmapNotify event so we can forget about the window later. */
	focuswin->iconic = true;

	xcb_unmap_window(conn, focuswin->id);
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, focuswin->id,
			    ewmh->_NET_WM_STATE, ewmh->_NET_WM_STATE, 32, 3, data);

	xcb_flush(conn);
}

bool
getpointer(const xcb_drawable_t *win, int16_t *x, int16_t *y)
{
	xcb_query_pointer_reply_t *pointer;

	pointer = xcb_query_pointer_reply(conn,
					  xcb_query_pointer(conn, *win), 0);

	*x = pointer->win_x;
	*y = pointer->win_y;

	free(pointer);

	if (NULL == pointer)
		return false;

	return true;
}

bool
getgeom(const xcb_drawable_t *win, int16_t *x, int16_t *y, uint16_t *width,
	uint16_t *height)
{
	xcb_get_geometry_reply_t *geom = xcb_get_geometry_reply(conn,
								xcb_get_geometry(conn, *win), NULL);

	if (NULL == geom)
		return false;

	*x = geom->x;
	*y = geom->y;
	*width = geom->width;
	*height = geom->height;

	free(geom);
	return true;
}

void
deletewin()
{
	bool use_delete = false;
	xcb_icccm_get_wm_protocols_reply_t protocols;
	xcb_get_property_cookie_t cookie;

	if (NULL == focuswin)
		return;

	/* Check if WM_DELETE is supported.  */
	cookie = xcb_icccm_get_wm_protocols_unchecked(conn, focuswin->id,
						      ewmh->WM_PROTOCOLS);

	if (focuswin->id == top_win)
		top_win = 0;

	if (xcb_icccm_get_wm_protocols_reply(conn, cookie, &protocols, NULL)
	    == 1) {
		for (uint32_t i = 0; i < protocols.atoms_len; i++)
			if (protocols.atoms[i] == ATOM[wm_delete_window]) {
				xcb_client_message_event_t ev = {
					.response_type	= XCB_CLIENT_MESSAGE,
					.format		= 32,
					.sequence	= 0,
					.window		= focuswin->id,
					.type		= ewmh->WM_PROTOCOLS,
					.data.data32	= {
						ATOM[wm_delete_window],
						XCB_CURRENT_TIME
					}
				};

				xcb_send_event(conn, false, focuswin->id,
					       XCB_EVENT_MASK_NO_EVENT,
					       (char *)&ev
					       );

				use_delete = true;
				break;
			}

		xcb_icccm_get_wm_protocols_reply_wipe(&protocols);
	}
	if (!use_delete) xcb_kill_client(conn, focuswin->id);
}

void
changescreen(const Arg *arg)
{
	struct item *item;
	float xpercentage, ypercentage;

	if (NULL == focuswin || NULL == focuswin->monitor)
		return;

	if (arg->i == NEXT_SCREEN)
		item = focuswin->monitor->item->next;
	else
		item = focuswin->monitor->item->prev;

	if (NULL == item)
		return;

	xpercentage = (float)((focuswin->x - focuswin->monitor->x)
			      / (focuswin->monitor->width));
	ypercentage = (float)((focuswin->y - focuswin->monitor->y)
			      / (focuswin->monitor->height));

	focuswin->monitor = item->data;

	focuswin->x = focuswin->monitor->width * xpercentage
		      + focuswin->monitor->x + 0.5;
	focuswin->y = focuswin->monitor->height * ypercentage
		      + focuswin->monitor->y + 0.5;

	raise_current_window();
	fitonscreen(focuswin);
	movelim(focuswin);
	centerpointer(focuswin->id, focuswin);
}

/* wrapper to get xcb keysymbol from keycode */
static xcb_keysym_t
xcb_get_keysym(xcb_keycode_t keycode)
{
	xcb_key_symbols_t *keysyms;

	if (!(keysyms = xcb_key_symbols_alloc(conn)))
		return 0;

	xcb_keysym_t keysym = xcb_key_symbols_get_keysym(keysyms, keycode, 0);
	xcb_key_symbols_free(keysyms);

	return keysym;
}

void
circulaterequest(xcb_generic_event_t *ev)
{
	xcb_circulate_request_event_t *e = (xcb_circulate_request_event_t *)ev;

	/*
	 * Subwindow e->window to parent e->event is about to be restacked.
	 * Just do what was requested, e->place is either
	 * XCB_PLACE_ON_TOP or _ON_BOTTOM.
	 */
	xcb_circulate_window(conn, e->window, e->place);
}

void
handle_keypress(xcb_generic_event_t *e)
{
	xcb_key_press_event_t *ev = (xcb_key_press_event_t *)e;
	xcb_keysym_t keysym = xcb_get_keysym(ev->detail);

	for (unsigned int i = 0; i < LENGTH(keys); i++) {
		if (keysym == keys[i].keysym && CLEANMASK(keys[i].mod)
		    == CLEANMASK(ev->state) && keys[i].func) {
			keys[i].func(&keys[i].arg);
			break;
		}
	}
}

/* Helper function to configure a window. */
void
configwin(xcb_window_t win, uint16_t mask, const struct winconf *wc)
{
	uint32_t values[7];
	int8_t i = -1;

	if (mask & XCB_CONFIG_WINDOW_X) {
		mask |= XCB_CONFIG_WINDOW_X;
		i++;
		values[i] = wc->x;
	}

	if (mask & XCB_CONFIG_WINDOW_Y) {
		mask |= XCB_CONFIG_WINDOW_Y;
		i++;
		values[i] = wc->y;
	}

	if (mask & XCB_CONFIG_WINDOW_WIDTH) {
		mask |= XCB_CONFIG_WINDOW_WIDTH;
		i++;
		values[i] = wc->width;
	}

	if (mask & XCB_CONFIG_WINDOW_HEIGHT) {
		mask |= XCB_CONFIG_WINDOW_HEIGHT;
		i++;
		values[i] = wc->height;
	}

	if (mask & XCB_CONFIG_WINDOW_SIBLING) {
		mask |= XCB_CONFIG_WINDOW_SIBLING;
		i++;
		values[i] = wc->sibling;
	}

	if (mask & XCB_CONFIG_WINDOW_STACK_MODE) {
		mask |= XCB_CONFIG_WINDOW_STACK_MODE;
		i++;
		values[i] = wc->stackmode;
	}

	if (i == -1)
		return;

	xcb_configure_window(conn, win, mask, values);
	xcb_flush(conn);
}

void
configurerequest(xcb_generic_event_t *ev)
{
	xcb_configure_request_event_t *e = (xcb_configure_request_event_t *)ev;
	struct client *client;
	struct winconf wc;
	int16_t mon_x, mon_y;
	uint16_t mon_width, mon_height;
	uint32_t values[1];

	if ((client = findclient(&e->window))) { /* Find the client. */
		getmonsize(&mon_x, &mon_y, &mon_width, &mon_height, client);

		if (e->value_mask & XCB_CONFIG_WINDOW_WIDTH)
			if (!client->maxed && !client->hormaxed)
				client->width = e->width;

		if (e->value_mask & XCB_CONFIG_WINDOW_HEIGHT)
			if (!client->maxed && !client->vertmaxed)
				client->height = e->height;


		if (e->value_mask & XCB_CONFIG_WINDOW_X)
			if (!client->maxed && !client->hormaxed)
				client->x = e->x;

		if (e->value_mask & XCB_CONFIG_WINDOW_Y)
			if (!client->maxed && !client->vertmaxed)
				client->y = e->y;

		/* XXX Do we really need to pass on sibling and stack mode
		 * configuration? Do we want to? */
		if (e->value_mask & XCB_CONFIG_WINDOW_SIBLING) {
			values[0] = e->sibling;
			xcb_configure_window(conn, e->window,
					     XCB_CONFIG_WINDOW_SIBLING, values);
		}

		if (e->value_mask & XCB_CONFIG_WINDOW_STACK_MODE) {
			values[0] = e->stack_mode;
			xcb_configure_window(conn, e->window,
					     XCB_CONFIG_WINDOW_STACK_MODE, values);
		}

		/* Check if window fits on screen after resizing. */
		if (!client->maxed) {
			resizelim(client);
			movelim(client);
			fitonscreen(client);
		}
	} else {
		/* Unmapped window, pass all options except border width. */
		wc.x = e->x;
		wc.y = e->y;
		wc.width = e->width;
		wc.height = e->height;
		wc.sibling = e->sibling;
		wc.stackmode = e->stack_mode;

		configwin(e->window, e->value_mask, &wc);
	}
}

xcb_cursor_t
Create_Font_Cursor(xcb_connection_t *conn, uint16_t glyph)
{
	static xcb_font_t cursor_font;

	cursor_font = xcb_generate_id(conn);
	xcb_open_font(conn, cursor_font, strlen("cursor"), "cursor");
	xcb_cursor_t cursor = xcb_generate_id(conn);
	xcb_create_glyph_cursor(conn, cursor, cursor_font, cursor_font, glyph,
				glyph + 1, 0x3232, 0x3232, 0x3232, 0xeeee, 0xeeee, 0xeeec
				);

	return cursor;
}

struct client
create_back_win(void)
{
	struct client temp_win;
	uint32_t values[1] = { 0 };

	temp_win.id = xcb_generate_id(conn);
	xcb_create_window(conn,
	                        /* depth */
			  XCB_COPY_FROM_PARENT,
	                        /* window Id */
			  temp_win.id,
	                        /* parent window */
			  screen->root,
	                        /* x, y */
			  focuswin->x,
			  focuswin->y,
	                  /* width, height */
			  focuswin->width,
			  focuswin->height,
	                        /* border width */
			  0,
	                        /* class */
			  XCB_WINDOW_CLASS_INPUT_OUTPUT,
	                        /* visual */
			  screen->root_visual,
			  XCB_CW_BORDER_PIXEL,
			  values
			  );

	xcb_change_window_attributes(conn, temp_win.id,
				     XCB_CW_BACK_PIXEL, values);

	temp_win.x = focuswin->x;
	temp_win.y = focuswin->y;
	temp_win.width = focuswin->width;
	temp_win.fixed = focuswin->fixed;
	temp_win.height = focuswin->height;
	temp_win.width_inc = focuswin->width_inc;
	temp_win.height_inc = focuswin->height_inc;
	temp_win.base_width = focuswin->base_width;
	temp_win.base_height = focuswin->base_height;
	temp_win.monitor = focuswin->monitor;
	temp_win.min_height = focuswin->min_height;
	temp_win.min_width = focuswin->min_height;

	return temp_win;
}

static void
mousemotion(const Arg *arg)
{
	int16_t mx, my, winx, winy, winw, winh;
	xcb_query_pointer_reply_t *pointer;
	xcb_grab_pointer_reply_t *grab_reply;
	xcb_motion_notify_event_t *ev = NULL;
	xcb_generic_event_t *e = NULL;
	bool ungrab;

	pointer = xcb_query_pointer_reply(conn,
					  xcb_query_pointer(conn, screen->root), 0
					  );

	if (!pointer || focuswin->maxed) {
		free(pointer);
		return;
	}

	mx = pointer->root_x;
	my = pointer->root_y;
	winx = focuswin->x;
	winy = focuswin->y;
	winw = focuswin->width;
	winh = focuswin->height;

	xcb_cursor_t cursor;
	struct client example;
	raise_current_window();

	if (arg->i == WIN_MOVE) {
		cursor = Create_Font_Cursor(conn, 52);          /* fleur */
	} else {
		cursor = Create_Font_Cursor(conn, 120);         /* sizing */
		example = create_back_win();
		xcb_map_window(conn, example.id);
	}

	grab_reply = xcb_grab_pointer_reply(conn, xcb_grab_pointer(conn, 0,
								   screen->root, BUTTONMASK | XCB_EVENT_MASK_BUTTON_MOTION
								   | XCB_EVENT_MASK_POINTER_MOTION, XCB_GRAB_MODE_ASYNC,
								   XCB_GRAB_MODE_ASYNC, XCB_NONE, cursor, XCB_CURRENT_TIME)
					    , NULL
					    );

	if (grab_reply->status != XCB_GRAB_STATUS_SUCCESS) {
		free(grab_reply);

		if (arg->i == WIN_RESIZE)
			xcb_unmap_window(conn, example.id);

		return;
	}

	free(grab_reply);
	ungrab = false;

	do {
		if (NULL != e)
			free(e);

		while (!(e = xcb_wait_for_event(conn)))
			xcb_flush(conn);

		switch (e->response_type & ~0x80) {
		case XCB_CONFIGURE_REQUEST:
		case XCB_MAP_REQUEST:
			events[e->response_type & ~0x80](e);
			break;
		case XCB_MOTION_NOTIFY:
			ev = (xcb_motion_notify_event_t *)e;
			if (arg->i == WIN_MOVE)
				mousemove(winx + ev->root_x - mx,
					  winy + ev->root_y - my);
			else
				mouseresize(&example, winw + ev->root_x - mx,
					    winh + ev->root_y - my);

			xcb_flush(conn);
			break;
		case XCB_KEY_PRESS:
		case XCB_KEY_RELEASE:
		case XCB_BUTTON_PRESS:
		case XCB_BUTTON_RELEASE:
			if (arg->i == WIN_RESIZE) {
				ev = (xcb_motion_notify_event_t *)e;

				mouseresize(focuswin, winw + ev->root_x - mx,
					    winh + ev->root_y - my);
			}

			ungrab = true;
			break;
		}
	} while (!ungrab && focuswin != NULL);

	free(pointer);
	free(e);
	xcb_free_cursor(conn, cursor);
	xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);

	if (arg->i == WIN_RESIZE)
		xcb_unmap_window(conn, example.id);

	xcb_flush(conn);
}

void
buttonpress(xcb_generic_event_t *ev)
{
	xcb_button_press_event_t *e = (xcb_button_press_event_t *)ev;
	unsigned int i;

	for (i = 0; i < LENGTH(buttons); i++)
		if (buttons[i].func && buttons[i].button == e->detail
		    && CLEANMASK(buttons[i].mask)
		    == CLEANMASK(e->state)) {
			if ((focuswin == NULL) && buttons[i].func == mousemotion)
				return;
			if (buttons[i].root_only) {
				if (e->event == e->root && e->child == 0)
					buttons[i].func(&(buttons[i].arg));
			} else {
				buttons[i].func(&(buttons[i].arg));
			}
		}
}

void
clientmessage(xcb_generic_event_t *ev)
{
	xcb_client_message_event_t *e = (xcb_client_message_event_t *)ev;
	struct client *cl;

	if ((e->type == ATOM[wm_change_state] && e->format == 32
	     && e->data.data32[0] == XCB_ICCCM_WM_STATE_ICONIC)
	    || e->type == ewmh->_NET_ACTIVE_WINDOW) {
		cl = findclient(&e->window);

		if (NULL == cl)
			return;

		if (false == cl->iconic) {
			if (e->type == ewmh->_NET_ACTIVE_WINDOW)
				setfocus(cl);
			else
				hide();

			return;
		}

		cl->iconic = false;
		xcb_map_window(conn, cl->id);
		setfocus(cl);
	}
}

void
destroynotify(xcb_generic_event_t *ev)
{
	struct client *cl;

	xcb_destroy_notify_event_t *e = (xcb_destroy_notify_event_t *)ev;

	if (NULL != focuswin && focuswin->id == e->window)
		focuswin = NULL;

	cl = findclient(&e->window);

	/* Find this window in list of clients and forget about it. */
	if (NULL != cl)
		forgetwin(cl->id);

	updateclientlist();
}

void
enternotify(xcb_generic_event_t *ev)
{
	xcb_enter_notify_event_t *e = (xcb_enter_notify_event_t *)ev;
	struct client *client;

	/*
	 * If this isn't a normal enter notify, don't bother. We also need
	 * ungrab events, since these will be generated on button and key
	 * grabs and if the user for some reason presses a button on the
	 * root and then moves the pointer to our window and releases the
	 * button, we get an Ungrab EnterNotify. The other cases means the
	 * pointer is grabbed and that either means someone is using it for
	 * menu selections or that we're moving or resizing. We don't want
	 * to change focus in those cases.
	 */

	if (e->mode == XCB_NOTIFY_MODE_NORMAL
	    || e->mode == XCB_NOTIFY_MODE_UNGRAB) {
		/* If we're entering the same window we focus now,
		 * then don't bother focusing. */

		if (NULL != focuswin && e->event == focuswin->id)
			return;

		/* Otherwise, set focus to the window we just entered if we
		 * can find it among the windows we know about.
		 * If not, just keep focus in the old window. */

		client = findclient(&e->event);
		if (NULL == client)
			return;

		setfocus(client);
	}
}

void
unmapnotify(xcb_generic_event_t *ev)
{
	xcb_unmap_notify_event_t *e = (xcb_unmap_notify_event_t *)ev;
	struct client *client = NULL;

	/*
	 * Find the window in our current workspace list, then forget about it.
	 * Note that we might not know about the window we got the UnmapNotify
	 * event for.
	 * It might be a window we just unmapped on *another* workspace when
	 * changing workspaces, for instance, or it might be a window with
	 * override redirect set.
	 * This is not an error.
	 * XXX We might need to look in the global window list, after all.
	 * Consider if a window is unmapped on our last workspace while
	 * changing workspaces.
	 * If we do this, we need to keep track of our own windows and
	 * ignore UnmapNotify on them.
	 */

	client = findclient(&e->window);
	if (NULL == client || client->wsitem == NULL)
		return;

	if (focuswin != NULL && client->id == focuswin->id)
		focuswin = NULL;

	if (client->iconic == false)
		forgetclient(client);

	updateclientlist();
}

void
confignotify(xcb_generic_event_t *ev)
{
	xcb_configure_notify_event_t *e = (xcb_configure_notify_event_t *)ev;

	if (e->window == screen->root) {
		/*
		 * When using RANDR or Xinerama, the root can change geometry
		 * when the user adds a new screen, tilts their screen 90
		 * degrees or whatnot. We might need to rearrange windows to
		 * be visible.
		 * We might get notified for several reasons, not just if the
		 * geometry changed.
		 * If the geometry is unchanged we do nothing.
		 */

		if (e->width != screen->width_in_pixels
		    || e->height != screen->height_in_pixels) {
			screen->width_in_pixels = e->width;
			screen->height_in_pixels = e->height;

			if (-1 == randrbase)
				arrangewindows();
		}
	}
}

void
run(void)
{
	xcb_generic_event_t *ev;

	sigcode = 0;

	while (0 == sigcode) {
		/* the WM is running */
		xcb_flush(conn);

		if (xcb_connection_has_error(conn))
			abort();

		if ((ev = xcb_wait_for_event(conn))) {
			if (ev->response_type == randrbase +
			    XCB_RANDR_SCREEN_CHANGE_NOTIFY)
				getrandr();

			if (events[ev->response_type & ~0x80])
				events[ev->response_type & ~0x80](ev);

			if (top_win != 0)
				raisewindow(top_win);

			free(ev);
		}
	}
}

/* Get a defined atom from the X server. */
xcb_atom_t
getatom(const char *atom_name)
{
	xcb_intern_atom_cookie_t atom_cookie = xcb_intern_atom(conn, 0,
							       strlen(atom_name), atom_name);

	xcb_intern_atom_reply_t *rep = xcb_intern_atom_reply(conn, atom_cookie,
							     NULL);

	/* XXX Note that we return 0 as an atom if anything goes wrong.
	 * Might become interesting.*/

	if (NULL == rep)
		return 0;

	xcb_atom_t atom = rep->atom;

	free(rep);
	return atom;
}

/* set the given client to listen to button events (presses / releases) */
void
grabbuttons(struct client *c)
{
	unsigned int modifiers[] = {
		0,
		XCB_MOD_MASK_LOCK,
		numlockmask,
		numlockmask | XCB_MOD_MASK_LOCK
	};

	for (unsigned int b = 0; b < LENGTH(buttons); b++)
		if (!buttons[b].root_only) {
			for (unsigned int m = 0; m < LENGTH(modifiers); m++)
				xcb_grab_button(conn, 1, c->id,
						XCB_EVENT_MASK_BUTTON_PRESS,
						XCB_GRAB_MODE_ASYNC,
						XCB_GRAB_MODE_ASYNC,
						screen->root, XCB_NONE,
						buttons[b].button,
						buttons[b].mask | modifiers[m]
						);
		}
}

void
ewmh_init(void)
{
	if (!(ewmh = calloc(1, sizeof(xcb_ewmh_connection_t))))
		printf("Fail\n");

	xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(conn, ewmh);
	xcb_ewmh_init_atoms_replies(ewmh, cookie, (void *)0);
}

bool
setup(int scrno)
{
	unsigned int i;

	unsigned int values[1] = {
		XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
		| XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
		| XCB_EVENT_MASK_PROPERTY_CHANGE
		| XCB_EVENT_MASK_BUTTON_PRESS
	};

	screen = xcb_screen_of_display(conn, scrno);

	if (!screen)
		return false;

	ewmh_init();
	xcb_ewmh_set_wm_pid(ewmh, screen->root, getpid());
	xcb_ewmh_set_wm_name(ewmh, screen->root, 4, "jwm");

	xcb_atom_t net_atoms[] = {
		ewmh->_NET_SUPPORTED,		   ewmh->_NET_WM_DESKTOP,
		ewmh->_NET_NUMBER_OF_DESKTOPS,	   ewmh->_NET_CURRENT_DESKTOP,
		ewmh->_NET_ACTIVE_WINDOW,	   ewmh->_NET_WM_ICON,
		ewmh->_NET_WM_STATE,		   ewmh->_NET_WM_NAME,
		ewmh->_NET_SUPPORTING_WM_CHECK,	   ewmh->_NET_WM_STATE_HIDDEN,
		ewmh->_NET_WM_ICON_NAME,	   ewmh->_NET_WM_WINDOW_TYPE,
		ewmh->_NET_WM_WINDOW_TYPE_DOCK,	   ewmh->_NET_WM_WINDOW_TYPE_DESKTOP,
		ewmh->_NET_WM_WINDOW_TYPE_TOOLBAR, ewmh->_NET_WM_PID,
		ewmh->_NET_CLIENT_LIST,		   ewmh->_NET_CLIENT_LIST_STACKING,
		ewmh->WM_PROTOCOLS,		   ewmh->_NET_WM_STATE,
		ewmh->_NET_WM_STATE_DEMANDS_ATTENTION
	};

	xcb_ewmh_set_supported(ewmh, scrno, LENGTH(net_atoms), net_atoms);

	for (i = 0; i < NB_ATOMS; i++)
		ATOM[i] = getatom(atomnames[i][0]);

	randrbase = setuprandr();

	if (!setup_keyboard())
		return false;

	xcb_generic_error_t *error = xcb_request_check(conn,
						       xcb_change_window_attributes_checked(conn, screen->root,
											    XCB_CW_EVENT_MASK, values));
	xcb_flush(conn);

	if (error)
		return false;

	grabkeys();
	/* set events */
	for (i = 0; i < XCB_NO_OPERATION; i++)
		events[i] = NULL;

	events[XCB_CONFIGURE_REQUEST] = configurerequest;
	events[XCB_DESTROY_NOTIFY] = destroynotify;
	events[XCB_ENTER_NOTIFY] = enternotify;
	events[XCB_KEY_PRESS] = handle_keypress;
	events[XCB_MAP_REQUEST] = newwin;
	events[XCB_UNMAP_NOTIFY] = unmapnotify;
	events[XCB_CONFIGURE_NOTIFY] = confignotify;
	events[XCB_CIRCULATE_REQUEST] = circulaterequest;
	events[XCB_BUTTON_PRESS] = buttonpress;
	events[XCB_CLIENT_MESSAGE] = clientmessage;

	return true;
}

void
sigcatch(const int sig)
{
	sigcode = sig;
}

void
install_sig_handlers(void)
{
	struct sigaction sa;

	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_NOCLDSTOP;
	//could not initialize signal handler
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
		exit(-1);
	sa.sa_handler = sigcatch;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 1; /* Restart if interrupted by handler */
	if (sigaction(SIGINT, &sa, NULL) == -1
	    || sigaction(SIGHUP, &sa, NULL) == -1
	    || sigaction(SIGTERM, &sa, NULL) == -1)
		exit(-1);
}

int
main(int argc, char **argv)
{
	int scrno;

	install_sig_handlers();
	atexit(cleanup);
	if (!xcb_connection_has_error(conn = xcb_connect(NULL, &scrno)))
		if (setup(scrno))
			run();
	/* the WM has stopped running, because sigcode is not 0 */
	exit(sigcode);
}
