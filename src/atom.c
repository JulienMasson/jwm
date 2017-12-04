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

#include <string.h>
#include <stdlib.h>

#include "global.h"
#include "atom.h"

static const char *atomnames[NB_ATOMS][1] = {
	{ "WM_DELETE_WINDOW" },
	{ "WM_CHANGE_STATE"  }
};
xcb_atom_t ATOM[NB_ATOMS];

/* Get a defined atom from the X server. */
static xcb_atom_t getatom(const char *atom_name)
{
	xcb_intern_atom_cookie_t atom_cookie = xcb_intern_atom(conn, 0,
							       strlen(atom_name), atom_name);

	xcb_intern_atom_reply_t *rep = xcb_intern_atom_reply(conn, atom_cookie,
							     NULL);

	/* XXX Note that we return 0 as an atom if anything goes wrong.
	 * Might become interesting.*/

	if (rep == NULL)
		return 0;

	xcb_atom_t atom = rep->atom;

	free(rep);
	return atom;
}

void atom_init(void)
{
	unsigned int i;
	for (i = 0; i < NB_ATOMS; i++)
		ATOM[i] = getatom(atomnames[i][0]);
}

xcb_atom_t atom_get(int atom)
{
	return ATOM[atom];
}
