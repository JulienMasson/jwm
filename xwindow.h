#ifndef XWINDOW_H
#define XWINDOW_H

#include <X11/Xlib.h>

#include "jwm.h"

Client *wintoclient(Window w);
Monitor *wintomon(Window w);
int xerror(Display *dpy, XErrorEvent *ee);
int xerrordummy(Display *dpy, XErrorEvent *ee);

#endif
