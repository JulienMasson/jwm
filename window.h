#ifndef XWINDOW_H
#define XWINDOW_H

#include <X11/Xlib.h>

#include "jwm.h"

Monitor *recttomon(int x, int y, int w, int h);
void tile(Monitor *m);
void monocle(Monitor *m);
void spawn(const Arg *arg);
void focusstack(const Arg *arg);
void killclient(const Arg *arg);
void nextlayout(const Arg *arg);
void movemouse(const Arg *arg);
void resizemouse(const Arg *arg);
void view(const Arg *arg);
void tag(const Arg *arg);
void quit(const Arg *arg);
void unmanage(Client *c, int destroyed);
void manage(Window w, XWindowAttributes *wa);
void cleanupmon(Monitor *mon);
long getstate(Window w);
int updategeom(void);
void togglefloating(const Arg *arg);
Monitor *wintomon(Window w);
Client *wintoclient(Window w);
void arrange(Monitor *m);
int gettextprop(Window w, Atom atom, char *text, unsigned int size);
int xerrordummy(Display *dpy, XErrorEvent *ee);
int xerror(Display *dpy, XErrorEvent *ee);
int running_state(void);

#endif
