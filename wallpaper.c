#include <Imlib2.h>
#include <X11/extensions/Xinerama.h>
#include <stdio.h>
#include "wallpaper.h"
#include "extern.h"
#include "screen.h"


static void
set_background_scaled(Pixmap pmap, int x, int y, int w, int h)
{
	Imlib_Image im = imlib_load_image(WALLPAPER_PATH);
	if (im) {
			imlib_context_set_image(im);
			imlib_context_set_drawable(pmap);
			imlib_context_set_anti_alias(1);
			imlib_context_set_dither(1);
			imlib_context_set_blend(0);
			imlib_context_set_angle(0);
			imlib_render_image_on_drawable_at_size(x, y, w, h);
			imlib_free_image();
	}
}

void
init_wallpaper(void)
{
	Visual *vis = NULL;
	Colormap cm;
	vis = DefaultVisual(dpy, DefaultScreen(dpy));
	cm = DefaultColormap(dpy, DefaultScreen(dpy));

	imlib_context_set_display(dpy);
	imlib_context_set_visual(vis);
	imlib_context_set_colormap(cm);
	imlib_context_set_color_modifier(NULL);
	imlib_context_set_progress_function(NULL);
	imlib_context_set_operation(IMLIB_OP_COPY);
}

void
set_wallpaper(void)
{
	int depth;
	XGCValues gcvalues;
	GC gc;
	int i;
	Pixmap pmap_d1, pmap_d2;
	XineramaScreenInfo *xinerama_screens = NULL;
	int num_xinerama_screens;
	Display *disp2;
	Window root2;
	int depth2;
	int sh = get_screen()->height;
	int sw = get_screen()->width;

	depth = DefaultDepth(dpy, DefaultScreen(dpy));
	pmap_d1 = XCreatePixmap(dpy, root, sw, sh, depth);

	if (XineramaIsActive(dpy))
		xinerama_screens = XineramaQueryScreens(dpy, &num_xinerama_screens);

	if (xinerama_screens) {
		for (i = 0; i < num_xinerama_screens; i++) {
			set_background_scaled(pmap_d1,
					     xinerama_screens[i].x_org, xinerama_screens[i].y_org,
					     xinerama_screens[i].width, xinerama_screens[i].height);
		}
		XFree(xinerama_screens);
	}
	else
		set_background_scaled(pmap_d1, 0, 0, sw, sh);

	/* create new display, copy pixmap to new display */
	disp2 = XOpenDisplay(NULL);
	if (!disp2)
		printf("Can't reopen X display.");
	root2 = RootWindow(disp2, DefaultScreen(disp2));
	depth2 = DefaultDepth(disp2, DefaultScreen(disp2));
	XSync(dpy, False);
	pmap_d2 = XCreatePixmap(disp2, root2, sw, sh, depth2);
	gcvalues.fill_style = FillTiled;
	gcvalues.tile = pmap_d1;
	gc = XCreateGC(disp2, pmap_d2, GCFillStyle | GCTile, &gcvalues);
	XFillRectangle(disp2, pmap_d2, gc, 0, 0, sw, sh);
	XFreeGC(disp2, gc);
	XSync(disp2, False);
	XSync(dpy, False);
	XFreePixmap(dpy, pmap_d1);

	XSetWindowBackgroundPixmap(disp2, root2, pmap_d2);
	XClearWindow(disp2, root2);
	XFlush(disp2);
	XSetCloseDownMode(disp2, RetainPermanent);
	XCloseDisplay(disp2);
}
