#ifndef EXTERN_H
#define EXTERN_H

#include "jwm.h"
#include "key.h"
#include "atom.h"

/* tags */
extern const char *tags[4];

/* Layouts */
extern const Layout layouts[3];

/* jwm vars */
extern Atom wmatom[WMLast], netatom[NetLast];
extern Display *dpy;
extern Drw *drw;
extern Monitor *mons, *selmon;
extern Window root;

#endif
