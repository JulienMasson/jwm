#include "screen.h"
#include "util.h"
#include "config.h"
#include "drw.h"

/* extern vars */
extern int screen;
extern int sw, sh;
extern Drw *drw;
extern Cur *cursor[CurLast];
extern Display *dpy;
extern Window root;


void
setup_screen(void)
{
	/* get screen properties */
	screen = DefaultScreen(dpy);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);

	/* get root window */
	root = RootWindow(dpy, screen);

	/* create default screen */
	drw = drw_create(dpy, screen, root, sw, sh);

	/* create font */
	if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
		die("no fonts could be loaded.");

	/* init cursors */
	cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
	cursor[CurResize] = drw_cur_create(drw, XC_sizing);
	cursor[CurMove] = drw_cur_create(drw, XC_fleur);
}
