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

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "global.h"
#include "utils.h"

/* Ewmh Connection. */
xcb_ewmh_connection_t *ewmh;

void ewmh_init(int scrno)
{
	if (!(ewmh = calloc(1, sizeof(xcb_ewmh_connection_t))))
		printf("Fail\n");

	xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(conn, ewmh);
	xcb_ewmh_init_atoms_replies(ewmh, cookie, (void *)0);

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
}

void ewmh_exit(void)
{
	xcb_ewmh_connection_wipe(ewmh);
	if (ewmh != NULL)
		free(ewmh);
}
