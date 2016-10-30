#include "bar.h"
#include "drw.h"
#include "extern.h"
#include "color.h"
#include "screen.h"
#include "util.h"
#include "widgets.h"
#include "layout.h"

/* Macros */
#define ColBorder               2

/* Static vars */
static Scm scheme[SchemeLast];
static const char *colors[SchemeLast][3]      = {
	/*                fg         bg         border   */
	[SchemeNorm] =  { col_gray3, col_gray1, col_gray2 },
 	[SchemeSel] =   { col_gray4, col_cyan,  col_cyan  },
	[SchemeTitle] = { col_cyan,  col_gray1, col_cyan  },
};
static int bar_height;
static int bar_width;
static int lrpad;            /* sum of left and right padding for text */

/* Global vars */
const char *tags[4] = { "Emacs", "Web", "Term", "Extras"};


void
init_bars_properties(void)
{
	lrpad = drw->fonts->h;
	bar_height = lrpad + 2;
}

void
setup_bars(void)
{
	updatebars();
	updatestatus();

	/* init appearance */
	scheme[SchemeNorm] = drw_scm_create(drw, colors[SchemeNorm], 3);
	scheme[SchemeSel] = drw_scm_create(drw, colors[SchemeSel], 3);
	scheme[SchemeTitle] = drw_scm_create(drw, colors[SchemeTitle], 3);
}

void
drawbar(Monitor *m)
{
	static char stext[256];
	int x, w, sw = 0;
	int boxs = drw->fonts->h / 9;
	int boxw = drw->fonts->h / 6 + 2;
	unsigned int i, occ = 0, urg = 0;
	Client *c;
	Layout layout = get_layout(m->current_layout);

	/* add widgets */
	get_widgets(stext, sizeof(stext));

	/* draw status bar */
	drw_setscheme(drw, scheme[SchemeNorm]);
	sw = text_width(stext) - lrpad / 2; /* no right padding so status text hugs the corner */
	drw_text(drw, m->ww - sw, 0, sw, bar_height, lrpad / 2 - 2, stext, 0);

	/* draw tags */
	for (c = m->clients; c; c = c->next) {
		occ |= c->tags;
		if (c->isurgent)
			urg |= c->tags;
	}
	x = 0;
	for (i = 0; i < LENGTH(tags); i++) {
		w = text_width(tags[i]);
		drw_setscheme(drw, scheme[m->current_tag & 1 << i ? SchemeSel : SchemeNorm]);
		drw_text(drw, x, 0, w, bar_height, lrpad / 2, tags[i], urg & 1 << i);
		if (occ & 1 << i)
			drw_rect(drw, x + boxs, boxs, boxw, boxw,
			         m == selmon && selmon->sel && selmon->sel->tags & 1 << i,
			         urg & 1 << i);
		x += w;
	}

	/* draw layouts */
	w = bar_width = text_width(layout.symbol);
	drw_setscheme(drw, scheme[SchemeNorm]);
	x = drw_text(drw, x, 0, w, bar_height, lrpad / 2, layout.symbol, 0);

	/* draw windows title */
	if ((w = m->ww - sw - x) > bar_height) {
		if (m->sel) {
			drw_setscheme(drw, scheme[m == selmon ? SchemeTitle : SchemeNorm]);
			drw_text(drw, x, 0, w, bar_height, lrpad / 2, m->sel->name, 0);
			if (m->sel->isfloating)
				drw_rect(drw, x + boxs, boxs, boxw, boxw, m->sel->isfixed, 0);
		} else {
			drw_setscheme(drw, scheme[SchemeNorm]);
			drw_rect(drw, x, 0, w, bar_height, 1, 1);
		}
	}
	drw_map(drw, m->barwin, 0, 0, m->ww, bar_height);
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
	m->wh -= bar_height;
	m->by = m->wy;
	m->wy = m->wy + bar_height;
}

void
updatebars(void)
{
	int snumber = get_screen()->number;
	Monitor *m;
	XSetWindowAttributes wa = {
		.override_redirect = True,
		.background_pixmap = ParentRelative,
		.event_mask = ButtonPressMask|ExposureMask
	};
	for (m = mons; m; m = m->next) {
		if (m->barwin)
			continue;
		m->barwin = XCreateWindow(dpy, root, m->wx, m->by, m->ww, bar_height, 0, DefaultDepth(dpy, snumber),
		                          CopyFromParent, DefaultVisual(dpy, snumber),
		                          CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
		XDefineCursor(dpy, m->barwin, get_cursor(CurNormal));
		XMapRaised(dpy, m->barwin);
	}
}

void
updatestatus(void)
{
	drawbar(selmon);
}

unsigned int
text_width(const char *text)
{
	return drw_fontset_getwidth(drw, text) + lrpad;
}

int
click_on_bar(int x)
{
	unsigned int i = 0, index = 0;
	do
		index += text_width(tags[i]);
	while (x >= index && ++i < LENGTH(tags));
	if (i < LENGTH(tags))
		return i;
	else
		return -1;
}

int
get_bar_height(void)
{
	return bar_height;
}

int
get_bar_width(void)
{
	return bar_width;
}

unsigned long
get_scheme_pixel(scheme_t type)
{
	return scheme[type][ColBorder].pixel;
}

void
cleanup_bar(void)
{
	size_t i;
	for (i = 0; i < SchemeLast; i++)
		free(scheme[i]);
}
