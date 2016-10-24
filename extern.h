#ifndef EXTERN_H
#define EXTERN_H

#include "jwm.h"

/* appearance */
extern const unsigned int borderpx;
extern const unsigned int snap;
extern const char *fonts[1];
extern const char *colors[SchemeLast][3];
extern const char *tags[4];

/* layout(s) */
extern const float mfact;
extern const int nmaster;
extern const int resizehints;
extern const Layout layouts[3];

/* commands */
extern Key keys[19];
extern const char *dmenucmd[];
extern char dmenumon[2];

/* button definitions */
extern Button buttons[4];

/* jwm vars */
extern const char broken[];
extern int screen;
extern int sw, sh;
extern int bh, blw;
extern int lrpad;
extern unsigned int numlockmask;
extern Atom wmatom[WMLast], netatom[NetLast];
extern int running;
extern Cur *cursor[CurLast];
extern Scm scheme[SchemeLast];
extern Display *dpy;
extern Drw *drw;
extern Monitor *mons, *selmon;
extern Window root;

#endif
