#include "cursor.h"
#include "drw.h"
#include "extern.h"


/* Static vars */
static Cur *cursor[CurLast];


void
init_cursors(void)
{
	cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
	cursor[CurResize] = drw_cur_create(drw, XC_sizing);
	cursor[CurMove] = drw_cur_create(drw, XC_fleur);
}

void
cleanup_cursors(void)
{
	size_t i;
	for (i = 0; i < CurLast; i++)
		drw_cur_free(drw, cursor[i]);
}

Cursor
get_cursor(cursor_t type)
{
	return cursor[type]->cursor;
}
