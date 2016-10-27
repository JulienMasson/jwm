#ifndef JWM_H
#define JWM_H

#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>

#include "drw.h"

/* macros */
#define ISVISIBLE(C)            ((C->tags & C->mon->current_tag))
#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)

/* struct definition */
typedef struct Monitor Monitor;
typedef struct Client Client;
struct Client {
	char name[256];
	float mina, maxa;
	int x, y, w, h;
	int oldx, oldy, oldw, oldh;
	int basew, baseh, incw, inch, maxw, maxh, minw, minh;
	int bw, oldbw;
	unsigned int tags;
	int isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen;
	Client *next;
	Client *snext;
	Monitor *mon;
	Window win;
};

typedef struct {
	const char *symbol;
	void (*arrange)(Monitor *);
} Layout;

struct Monitor {
	char ltsymbol[16];
	int nmaster;
	int num;
	int by;               /* bar geometry */
	int mx, my, mw, mh;   /* screen size */
	int wx, wy, ww, wh;   /* window area  */
	unsigned int current_tag;
	unsigned int current_layout;
	Client *clients;
	Client *sel;
	Client *stack;
	Monitor *next;
	Window barwin;
};

typedef union {
	int i;
	unsigned int ui;
	float f;
	const void *v;
} Arg;

#endif
