#ifndef XWINDOW_H
#define XWINDOW_H

#include <X11/Xlib.h>

#include "jwm.h"
#include "client.h"

typedef enum {
	floating,
	tiling,
	tab,
	last_layout,
} layout_t;

typedef struct Monitor Monitor;
struct Monitor {
	int num;
	int by;               /* bar geometry */
	int mx, my, mw, mh;   /* screen size */
	int wx, wy, ww, wh;   /* window area  */
	unsigned int current_tag;
	layout_t current_layout;
	Client *clients;
	Client *sel;
	Client *stack;
	Monitor *next;
	Window barwin;
};

typedef struct {
	const char *symbol;
	void (*arrange)(Monitor *);
} Layout;

#define ISVISIBLE(C,M)          ((C->tags & M->current_tag))

Monitor *get_monitor_from_client(Client *c);
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

int applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
void attach(Monitor *m, Client *c);
void detach(Monitor *m, Client *c);
void attachstack(Monitor *m, Client *c);
void detachstack(Monitor *m, Client *c);
void focus(Monitor *m, Client *c);
void showhide(Monitor *m, Client *c);
void setfullscreen(Client *c, int fullscreen);
void pop(Client *c);
void updateclientlist(void);

#endif
