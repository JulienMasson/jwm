#include "screen.h"
#include "util.h"
#include "extern.h"
#include "drw.h"
#include "font.h"

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

}
