#include "event.h"
#include "jwm.h"
#include "drw.h"

/* extern vars */
extern Display *dpy;
extern Window root;
extern Cur *cursor[CurLast];


void
setup_events(void)
{
	XSetWindowAttributes wa;
	wa.cursor = cursor[CurNormal]->cursor;
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask|ButtonPressMask|PointerMotionMask
	                |EnterWindowMask|LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;
	XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
	XSelectInput(dpy, root, wa.event_mask);
}
