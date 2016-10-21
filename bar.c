#include <time.h>

#include "bar.h"
#include "drw.h"

extern Drw *drw;
extern Monitor *mons, *selmon;
extern Scm scheme[SchemeLast];
extern int lrpad;            /* sum of left and right padding for text */
extern int bh,blw;      /* bar geometry */
extern Display *dpy;
extern Window root;
extern int screen;
extern Cur *cursor[CurLast];

static const char *tags[] = { "Emacs", "Web", "Term", "Extras"};

void
drawbar(Monitor *m)
{
	static char stext[256];
	int x, w, sw = 0;
	int boxs = drw->fonts->h / 9;
	int boxw = drw->fonts->h / 6 + 2;
	unsigned int i, occ = 0, urg = 0;
	Client *c;
	time_t t;
	t = time(NULL);
	sprintf(stext, "%s", ctime(&t));

	/* draw status first so it can be overdrawn by tags later */
	if (m == selmon) { /* status is only drawn on selected monitor */
		drw_setscheme(drw, scheme[SchemeNorm]);
		sw = TEXTW(stext) - lrpad / 2; /* no right padding so status text hugs the corner */
		drw_text(drw, m->ww - sw, 0, sw, bh, lrpad / 2 - 2, stext, 0);
	}

	for (c = m->clients; c; c = c->next) {
		occ |= c->tags;
		if (c->isurgent)
			urg |= c->tags;
	}
	x = 0;
	for (i = 0; i < LENGTH(tags); i++) {
		w = TEXTW(tags[i]);
		drw_setscheme(drw, scheme[m->tagset[m->seltags] & 1 << i ? SchemeSel : SchemeNorm]);
		drw_text(drw, x, 0, w, bh, lrpad / 2, tags[i], urg & 1 << i);
		if (occ & 1 << i)
			drw_rect(drw, x + boxs, boxs, boxw, boxw,
			         m == selmon && selmon->sel && selmon->sel->tags & 1 << i,
			         urg & 1 << i);
		x += w;
	}
	w = blw = TEXTW(m->ltsymbol);
	drw_setscheme(drw, scheme[SchemeNorm]);
	x = drw_text(drw, x, 0, w, bh, lrpad / 2, m->ltsymbol, 0);

	if ((w = m->ww - sw - x) > bh) {
		if (m->sel) {
			drw_setscheme(drw, scheme[m == selmon ? SchemeSel : SchemeNorm]);
			drw_text(drw, x, 0, w, bh, lrpad / 2, m->sel->name, 0);
			if (m->sel->isfloating)
				drw_rect(drw, x + boxs, boxs, boxw, boxw, m->sel->isfixed, 0);
		} else {
			drw_setscheme(drw, scheme[SchemeNorm]);
			drw_rect(drw, x, 0, w, bh, 1, 1);
		}
	}
	drw_map(drw, m->barwin, 0, 0, m->ww, bh);
}

void
drawbars(void)
{
	Monitor *m;

	for (m = mons; m; m = m->next)
		drawbar(m);
}

void
updatebarpos(Monitor *m)
{
	m->wy = m->my;
	m->wh = m->mh;
	m->wh -= bh;
	m->by = m->wy;
	m->wy = m->wy + bh;
}

void
updatebars(void)
{
	Monitor *m;
	XSetWindowAttributes wa = {
		.override_redirect = True,
		.background_pixmap = ParentRelative,
		.event_mask = ButtonPressMask|ExposureMask
	};
	for (m = mons; m; m = m->next) {
		if (m->barwin)
			continue;
		m->barwin = XCreateWindow(dpy, root, m->wx, m->by, m->ww, bh, 0, DefaultDepth(dpy, screen),
		                          CopyFromParent, DefaultVisual(dpy, screen),
		                          CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
		XDefineCursor(dpy, m->barwin, cursor[CurNormal]->cursor);
		XMapRaised(dpy, m->barwin);
	}
}

void
updatestatus(void)
{
	drawbar(selmon);
}
