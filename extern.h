#ifndef EXTERN_H
#define EXTERN_H

#include "jwm.h"
#include "key.h"

/* tags */
extern const char *tags[4];

/* Layouts */
extern const Layout layouts[3];

/* jwm vars */
extern int bh, blw;
extern int lrpad;
extern unsigned int numlockmask;
extern Atom wmatom[WMLast], netatom[NetLast];
extern int running;
extern Scm scheme[SchemeLast];
extern Display *dpy;
extern Drw *drw;
extern Monitor *mons, *selmon;
extern Window root;

#endif
