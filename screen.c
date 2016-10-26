#include "screen.h"
#include "util.h"
#include "extern.h"
#include "drw.h"
#include "font.h"

/* Global vars */
static screen_t screen;


void
setup_screen(void)
{
	int number, height, width;
	/* get screen properties */
	number = screen.number = DefaultScreen(dpy);
	height = screen.height = DisplayWidth(dpy, number);
	width = screen.width = DisplayHeight(dpy, number);

	/* get root window */
	root = RootWindow(dpy, number);

	/* create default screen */
	drw = drw_create(dpy, number, root, width, height);
}

screen_t *
get_screen(void)
{
	return &screen;
}
