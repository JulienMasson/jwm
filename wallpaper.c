#include <Imlib2.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>
#include <limits.h>
#include <X11/Xutil.h>
#include <string.h>
#include <stdio.h>
#include <X11/Xatom.h>

#include "wallpaper.h"

#define XY_IN_RECT(x, y, rx, ry, rw, rh)				\
	(((x) >= (rx)) && ((y) >= (ry)) && ((x) < ((rx) + (rw))) && ((y) < ((ry) + (rh))))

struct __fehoptions {
	unsigned char xinerama;
	int xinerama_index;
};
typedef struct __fehoptions fehoptions;

fehoptions opt;
Display *disp = NULL;
Visual *vis = NULL;
Screen *scr = NULL;
Colormap cm;
int depth;
Atom wmDeleteWindow;
XContext xid_context = 0;
Window root = 0;

/* Xinerama support */
XineramaScreenInfo *xinerama_screens = NULL;
int xinerama_screen;
int num_xinerama_screens;

void init_parse_options(void)
{
	/* Set default options */
	memset(&opt, 0, sizeof(fehoptions));

	/* if we're using xinerama, then enable it by default */
	opt.xinerama = 1;
	opt.xinerama_index = -1;

	return;
}

void init_xinerama(void)
{
	if (opt.xinerama && XineramaIsActive(disp)) {
		int major, minor, px, py, i;

		/* discarded */
		Window dw;
		int di;
		unsigned int du;

		XineramaQueryVersion(disp, &major, &minor);
		xinerama_screens = XineramaQueryScreens(disp, &num_xinerama_screens);

		if (opt.xinerama_index >= 0)
			xinerama_screen = opt.xinerama_index;
		else {
			xinerama_screen = 0;
			XQueryPointer(disp, root, &dw, &dw, &px, &py, &di, &di, &du);
			for (i = 0; i < num_xinerama_screens; i++) {
				if (XY_IN_RECT(px, py,
					       xinerama_screens[i].x_org,
					       xinerama_screens[i].y_org,
					       xinerama_screens[i].width,
					       xinerama_screens[i].height)) {
					xinerama_screen = i;
					break;
				}
			}
		}
	}
}

void init_x_and_imlib(void)
{
	disp = XOpenDisplay(NULL);
	if (!disp)
		printf("Can't open X display. It *is* running, yeah?");
	vis = DefaultVisual(disp, DefaultScreen(disp));
	depth = DefaultDepth(disp, DefaultScreen(disp));
	cm = DefaultColormap(disp, DefaultScreen(disp));
	root = RootWindow(disp, DefaultScreen(disp));
	scr = ScreenOfDisplay(disp, DefaultScreen(disp));
	xid_context = XUniqueContext();

	init_xinerama();

	imlib_context_set_display(disp);
	imlib_context_set_visual(vis);
	imlib_context_set_colormap(cm);
	imlib_context_set_color_modifier(NULL);
	imlib_context_set_progress_function(NULL);
	imlib_context_set_operation(IMLIB_OP_COPY);
	wmDeleteWindow = XInternAtom(disp, "WM_DELETE_WINDOW", False);

	return;
}

void
gib_imlib_render_image_on_drawable_at_size(Drawable d, Imlib_Image im, int x,
                                           int y, int w, int h, char dither,
                                           char blend, char alias)
{
	imlib_context_set_image(im);
	imlib_context_set_drawable(d);
	imlib_context_set_anti_alias(alias);
	imlib_context_set_dither(dither);
	imlib_context_set_blend(blend);
	imlib_context_set_angle(0);
	imlib_render_image_on_drawable_at_size(x, y, w, h);
}

static void feh_wm_set_bg_scaled(Pixmap pmap, Imlib_Image im, int use_filelist,
				 int x, int y, int w, int h)
{
	Imlib_Load_Error err = IMLIB_LOAD_ERROR_NONE;
	im = imlib_load_image_with_error_return("/home/julien/Documents/jwm/wallpaper.jpg", &err);
	/* im = imlib_load_image_with_error_return("/home/julien/Pictures/screenshot.png", &err); */
	gib_imlib_render_image_on_drawable_at_size(pmap, im, x, y, w, h,
						   1, 0, 1);

	return;
}

void feh_wm_set_bg(char *fil, Imlib_Image im, int centered, int scaled,
		   int filled, int desktop, int use_filelist)
{
	XGCValues gcvalues;
	XGCValues gcval;
	GC gc;

	/*
	 * TODO this re-implements mkstemp (badly). However, it is only needed
	 * for non-file images and enlightenment. Might be easier to just remove
	 * it.
	 */

	Atom prop_root, prop_esetroot, type;
	int format, i;
	unsigned long length, after;
	unsigned char *data_root = NULL, *data_esetroot = NULL;
	Pixmap pmap_d1, pmap_d2;

	/* local display to set closedownmode on */
	Display *disp2;
	Window root2;
	int depth2;

	pmap_d1 = XCreatePixmap(disp, root, scr->width, scr->height, depth);

	if (opt.xinerama_index >= 0) {
		gcval.foreground = BlackPixel(disp, DefaultScreen(disp));
		gc = XCreateGC(disp, root, GCForeground, &gcval);
		XFillRectangle(disp, pmap_d1, gc, 0, 0, scr->width, scr->height);
		XFreeGC(disp, gc);
	}

	if (opt.xinerama && xinerama_screens) {
		for (i = 0; i < num_xinerama_screens; i++) {
			if (opt.xinerama_index < 0 || opt.xinerama_index == i) {
				feh_wm_set_bg_scaled(pmap_d1, im, use_filelist,
						     xinerama_screens[i].x_org, xinerama_screens[i].y_org,
						     xinerama_screens[i].width, xinerama_screens[i].height);
			}
		}
	}
	else
		feh_wm_set_bg_scaled(pmap_d1, im, use_filelist,
				     0, 0, scr->width, scr->height);

	/* create new display, copy pixmap to new display */
	disp2 = XOpenDisplay(NULL);
	if (!disp2)
		printf("Can't reopen X display.");
	root2 = RootWindow(disp2, DefaultScreen(disp2));
	depth2 = DefaultDepth(disp2, DefaultScreen(disp2));
	XSync(disp, False);
	pmap_d2 = XCreatePixmap(disp2, root2, scr->width, scr->height, depth2);
	gcvalues.fill_style = FillTiled;
	gcvalues.tile = pmap_d1;
	gc = XCreateGC(disp2, pmap_d2, GCFillStyle | GCTile, &gcvalues);
	XFillRectangle(disp2, pmap_d2, gc, 0, 0, scr->width, scr->height);
	XFreeGC(disp2, gc);
	XSync(disp2, False);
	XSync(disp, False);
	XFreePixmap(disp, pmap_d1);

	prop_root = XInternAtom(disp2, "_XROOTPMAP_ID", True);
	prop_esetroot = XInternAtom(disp2, "ESETROOT_PMAP_ID", True);

	if (prop_root != None && prop_esetroot != None) {
		XGetWindowProperty(disp2, root2, prop_root, 0L, 1L,
				   False, AnyPropertyType, &type, &format, &length, &after, &data_root);
		if (type == XA_PIXMAP) {
			XGetWindowProperty(disp2, root2,
					   prop_esetroot, 0L, 1L,
					   False, AnyPropertyType,
					   &type, &format, &length, &after, &data_esetroot);
			if (data_root && data_esetroot) {
				if (type == XA_PIXMAP && *((Pixmap *) data_root) == *((Pixmap *) data_esetroot)) {
					XKillClient(disp2, *((Pixmap *)
							     data_root));
				}
			}
		}
	}

	if (data_root)
		XFree(data_root);

	if (data_esetroot)
		XFree(data_esetroot);

	/* This will locate the property, creating it if it doesn't exist */
	prop_root = XInternAtom(disp2, "_XROOTPMAP_ID", False);
	prop_esetroot = XInternAtom(disp2, "ESETROOT_PMAP_ID", False);

	if (prop_root == None || prop_esetroot == None)
		printf("creation of pixmap property failed.");

	XChangeProperty(disp2, root2, prop_root, XA_PIXMAP, 32, PropModeReplace, (unsigned char *) &pmap_d2, 1);
	XChangeProperty(disp2, root2, prop_esetroot, XA_PIXMAP, 32,
			PropModeReplace, (unsigned char *) &pmap_d2, 1);

	XSetWindowBackgroundPixmap(disp2, root2, pmap_d2);
	XClearWindow(disp2, root2);
	XFlush(disp2);
	XSetCloseDownMode(disp2, RetainPermanent);
	XCloseDisplay(disp2);
	return;
}


void
set_wallpaper(void)
{
	init_parse_options();

	init_x_and_imlib();

	feh_wm_set_bg(NULL, NULL, 0, 1, 0, 0, 1);
}
