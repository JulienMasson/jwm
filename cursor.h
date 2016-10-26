#ifndef CURSOR_H
#define CURSOR_H

#include <X11/X.h>

typedef struct {
	Cursor cursor;
} Cur;

typedef enum {
	CurNormal,
	CurResize,
	CurMove,
	CurLast
} cursor_t;

void init_cursors(void);
void cleanup_cursors(void);
Cursor get_cursor(cursor_t type);

#endif
