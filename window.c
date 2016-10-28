#include <X11/Xproto.h>
#include <X11/extensions/Xinerama.h>
#include <stdio.h>
#include <unistd.h>

#include "window.h"
#include "client.h"
#include "extern.h"
#include "util.h"
#include "event.h"
#include "bar.h"
#include "key.h"
#include "screen.h"
#include "layout.h"

/* Macros */
#define INTERSECT(x,y,w,h,m)    (MAX(0, MIN((x)+(w),(m)->wx+(m)->ww) - MAX((x),(m)->wx)) \
                               * MAX(0, MIN((y)+(h),(m)->wy+(m)->wh) - MAX((y),(m)->wy)))

/* static vars */
static const unsigned int snap     = 32;   /* snap pixel */
static const int nmaster           = 1;    /* number of clients in master area */
static int running = 1;
static int (*xerrorxlib)(Display *, XErrorEvent *);

Monitor *
get_monitor_from_client(Client *c)
{
	Monitor *m;
	Client *client_p;

	for (m = mons; m; m = m->next)
		for (client_p = m->clients; client_p; client_p = client_p->next)
			if (client_p == c)
				return m;
	return selmon;

}

int
applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact)
{
	int baseismin;
	Monitor *m = get_monitor_from_client(c);
	int sh = get_screen()->height;
	int sw = get_screen()->width;
	int bh = get_bar_height();

	/* set minimum possible */
	*w = MAX(1, *w);
	*h = MAX(1, *h);
	if (interact) {
		if (*x > sw)
			*x = sw - WIDTH(c);
		if (*y > sh)
			*y = sh - HEIGHT(c);
		if (*x + *w < 0)
			*x = 0;
		if (*y + *h < 0)
			*y = 0;
	} else {
		if (*x >= m->wx + m->ww)
			*x = m->wx + m->ww - WIDTH(c);
		if (*y >= m->wy + m->wh)
			*y = m->wy + m->wh - HEIGHT(c);
		if (*x + *w <= m->wx)
			*x = m->wx;
		if (*y + *h <= m->wy)
			*y = m->wy;
	}
	if (*h < bh)
		*h = bh;
	if (*w < bh)
		*w = bh;
	if (c->isfloating || (m->current_layout == floating)) {
		/* see last two sentences in ICCCM 4.1.2.3 */
		baseismin = c->basew == c->minw && c->baseh == c->minh;
		if (!baseismin) { /* temporarily remove base dimensions */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for aspect limits */
		if (c->mina > 0 && c->maxa > 0) {
			if (c->maxa < (float)*w / *h)
				*w = *h * c->maxa + 0.5;
			else if (c->mina < (float)*h / *w)
				*h = *w * c->mina + 0.5;
		}
		if (baseismin) { /* increment calculation requires this */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for increment value */
		if (c->incw)
			*w -= *w % c->incw;
		if (c->inch)
			*h -= *h % c->inch;
		/* restore base dimensions */
		*w = MAX(*w + c->basew, c->minw);
		*h = MAX(*h + c->baseh, c->minh);
		if (c->maxw)
			*w = MIN(*w, c->maxw);
		if (c->maxh)
			*h = MIN(*h, c->maxh);
	}
	return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

void
attach(Monitor *m, Client *c)
{
	c->next = m->clients;
	m->clients = c;
}

void
detach(Monitor *m, Client *c)
{
	Client **tc;

	for (tc = &m->clients; *tc && *tc != c; tc = &(*tc)->next);
	*tc = c->next;
}

void
attachstack(Monitor *m, Client *c)
{
	c->snext = m->stack;
	m->stack = c;
}

void
detachstack(Monitor *m, Client *c)
{
	Client **tc, *t;

	for (tc = &m->stack; *tc && *tc != c; tc = &(*tc)->snext);
	*tc = c->snext;

	if (c == m->sel) {
		for (t = m->stack; t && !ISVISIBLE(c,m); t = t->snext);
		m->sel = t;
	}
}

void
focus(Monitor *m, Client *c)
{
	if (!c || !ISVISIBLE(c,m))
		for (c = selmon->stack; c && !ISVISIBLE(c,m); c = c->snext);
	/* was if (selmon->sel) */
	if (selmon->sel && selmon->sel != c)
		unfocus(selmon->sel, 0);
	if (c) {
		if (m != selmon)
			selmon = m;
		if (c->isurgent)
			clearurgent(c);
		detachstack(m, c);
		attachstack(m, c);
		grabbuttons(c, 1);
		XSetWindowBorder(dpy, c->win, get_scheme_pixel(SchemeSel));
		setfocus(c);
	} else {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
	selmon->sel = c;
	drawbars();
}

void
pop(Client *c)
{
	Monitor *m = get_monitor_from_client(c);
	detach(m, c);
	attach(m, c);
	focus(m, c);
	arrange(m);
}

void
showhide(Monitor *m, Client *c)
{
	if (!c)
		return;
	if (ISVISIBLE(c,m)) {
		/* show clients top down */
		XMoveWindow(dpy, c->win, c->x, c->y);
		if (((m->current_layout == floating) || c->isfloating) && !c->isfullscreen)
			resize(c, c->x, c->y, c->w, c->h, 0);
		showhide(m, c->snext);
	} else {
		/* hide clients bottom up */
		showhide(m, c->snext);
		XMoveWindow(dpy, c->win, WIDTH(c) * -2, c->y);
	}
}

void
setfullscreen(Client *c, int fullscreen)
{
	Monitor *m = get_monitor_from_client(c);
	if (fullscreen && !c->isfullscreen) {
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
		                PropModeReplace, (unsigned char*)&netatom[NetWMFullscreen], 1);
		c->isfullscreen = 1;
		c->oldstate = c->isfloating;
		c->isfloating = 1;
		resizeclient(c, m->mx, m->my, m->mw, m->mh);
		XRaiseWindow(dpy, c->win);
	} else if (!fullscreen && c->isfullscreen){
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
		                PropModeReplace, (unsigned char*)0, 0);
		c->isfullscreen = 0;
		c->isfloating = c->oldstate;
		c->x = c->oldx;
		c->y = c->oldy;
		c->w = c->oldw;
		c->h = c->oldh;
		resizeclient(c, c->x, c->y, c->w, c->h);
		arrange(m);
	}
}

void
updateclientlist(void)
{
	Client *c;
	Monitor *m;

	XDeleteProperty(dpy, root, netatom[NetClientList]);
	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			XChangeProperty(dpy, root, netatom[NetClientList],
			                XA_WINDOW, 32, PropModeAppend,
			                (unsigned char *) &(c->win), 1);
}

int
getrootptr(int *x, int *y)
{
	int di;
	unsigned int dui;
	Window dummy;

	return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

Monitor *
recttomon(int x, int y, int w, int h)
{
	Monitor *m, *r = selmon;
	int a, area = 0;

	for (m = mons; m; m = m->next)
		if ((a = INTERSECT(x, y, w, h, m)) > area) {
			area = a;
			r = m;
		}
	return r;
}

Monitor *
createmon(void)
{
	Monitor *m;

	m = ecalloc(1, sizeof(Monitor));
	m->current_tag = 1;
	m->current_layout = floating;
	m->nmaster = nmaster;
	strncpy(m->ltsymbol, get_layout(m->current_layout).symbol, sizeof m->ltsymbol);
	return m;
}

Client *
wintoclient(Window w)
{
	Client *c;
	Monitor *m;

	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			if (c->win == w)
				return c;
	return NULL;
}

Monitor *
wintomon(Window w)
{
	int x, y;
	Client *c;
	Monitor *m;

	if (w == root && getrootptr(&x, &y))
		return recttomon(x, y, 1, 1);
	for (m = mons; m; m = m->next)
		if (w == m->barwin) /* bar window */
			return m;
		else
			for (c = m->clients; c; c = c->next) /* clients windows */
				if (w == c->win)
					return m;
	return selmon;
}

void
restack(Monitor *m)
{
	Client *c;
	XEvent ev;
	XWindowChanges wc;

	drawbar(m);
	if (!m->sel)
		return;
	if (m->sel->isfloating || (m->current_layout == floating))
		XRaiseWindow(dpy, m->sel->win);
	if (m->current_layout != floating) {
		wc.stack_mode = Below;
		wc.sibling = m->barwin;
		for (c = m->stack; c; c = c->snext)
			if (!c->isfloating && ISVISIBLE(c,m)) {
				XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);
				wc.sibling = c->win;
			}
	}
	XSync(dpy, False);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's).  Other types of errors call Xlibs
 * default error handler, which may call exit.  */
int
xerror(Display *dpy, XErrorEvent *ee)
{
	if (ee->error_code == BadWindow
	    || (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
	    || (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
	    || (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
	    || (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
	    || (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
	    || (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
	    || (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
	    || (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
		return 0;
	fprintf(stderr, "jwm: fatal error: request code=%d, error code=%d\n",
		ee->request_code, ee->error_code);
	return xerrorxlib(dpy, ee); /* may call exit */
}

int
xerrordummy(Display *dpy, XErrorEvent *ee)
{
	return 0;
}

int
isuniquegeom(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info)
{
	while (n--)
		if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org
		    && unique[n].width == info->width && unique[n].height == info->height)
			return 0;
	return 1;
}

int
updategeom(void)
{
	int dirty = 0;
	int sh = get_screen()->height;
	int sw = get_screen()->width;

	if (XineramaIsActive(dpy)) {
		int i, j, n, nn;
		Client *c;
		Monitor *m;
		XineramaScreenInfo *info = XineramaQueryScreens(dpy, &nn);
		XineramaScreenInfo *unique = NULL;

		for (n = 0, m = mons; m; m = m->next, n++);
		/* only consider unique geometries as separate screens */
		unique = ecalloc(nn, sizeof(XineramaScreenInfo));
		for (i = 0, j = 0; i < nn; i++)
			if (isuniquegeom(unique, j, &info[i]))
				memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
		XFree(info);
		nn = j;
		if (n <= nn) {
			for (i = 0; i < (nn - n); i++) { /* new monitors available */
				for (m = mons; m && m->next; m = m->next);
				if (m)
					m->next = createmon();
				else
					mons = createmon();
			}
			for (i = 0, m = mons; i < nn && m; m = m->next, i++)
				if (i >= n
				    || (unique[i].x_org != m->mx || unique[i].y_org != m->my
					|| unique[i].width != m->mw || unique[i].height != m->mh))
				{
					dirty = 1;
					m->num = i;
					m->mx = m->wx = unique[i].x_org;
					m->my = m->wy = unique[i].y_org;
					m->mw = m->ww = unique[i].width;
					m->mh = m->wh = unique[i].height;
					updatebarpos(m);
				}
		} else {
			/* less monitors available nn < n */
			for (i = nn; i < n; i++) {
				for (m = mons; m && m->next; m = m->next);
				while (m->clients) {
					dirty = 1;
					c = m->clients;
					m->clients = c->next;
					detachstack(m, c);
					attach(m, c);
					attachstack(m, c);
				}
				if (m == selmon)
					selmon = mons;
				cleanupmon(m);
			}
		}
		free(unique);
	} else
		/* default monitor setup */
	{
		if (!mons)
			mons = createmon();
		if (mons->mw != sw || mons->mh != sh) {
			dirty = 1;
			mons->mw = mons->ww = sw;
			mons->mh = mons->wh = sh;
			updatebarpos(mons);
		}
	}
	if (dirty) {
		selmon = mons;
		selmon = wintomon(root);
	}
	return dirty;
}

void
cleanupmon(Monitor *mon)
{
	Monitor *m;

	if (mon == mons)
		mons = mons->next;
	else {
		for (m = mons; m && m->next != mon; m = m->next);
		m->next = mon->next;
	}
	XUnmapWindow(dpy, mon->barwin);
	XDestroyWindow(dpy, mon->barwin);
	free(mon);
}

void
focusstack(const Arg *arg)
{
	Client *c = NULL, *i;
	Monitor *m = NULL;

	if (!selmon->sel)
		return;
	if (arg->i > 0) {
		for (c = selmon->sel->next; c; c = c->next) {
			m = get_monitor_from_client(c);
			if (ISVISIBLE(c,m))
				break;
		}
	} else {
		for (i = selmon->clients; i != selmon->sel; i = i->next) {
			m = get_monitor_from_client(i);
			if (ISVISIBLE(i,m))
				c = i;
		}
	}
	if (c) {
		focus(selmon, c);
		restack(selmon);
	}
}

long
getstate(Window w)
{
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;

	if (XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
			       &real, &format, &n, &extra, (unsigned char **)&p) != Success)
		return -1;
	if (n != 0)
		result = *p;
	XFree(p);
	return result;
}

int
gettextprop(Window w, Atom atom, char *text, unsigned int size)
{
	char **list = NULL;
	int n;
	XTextProperty name;

	if (!text || size == 0)
		return 0;
	text[0] = '\0';
	XGetTextProperty(dpy, w, &name, atom);
	if (!name.nitems)
		return 0;
	if (name.encoding == XA_STRING)
		strncpy(text, (char *)name.value, size - 1);
	else {
		if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 && *list) {
			strncpy(text, *list, size - 1);
			XFreeStringList(list);
		}
	}
	text[size - 1] = '\0';
	XFree(name.value);
	return 1;
}

void
arrangemon(Monitor *m)
{
	strncpy(m->ltsymbol, get_layout(m->current_layout).symbol, sizeof m->ltsymbol);
	get_layout(m->current_layout).arrange(m);
}

void
arrange(Monitor *m)
{
	if (m)
		showhide(m, m->stack);
	else for (m = mons; m; m = m->next)
		     showhide(m, m->stack);
	if (m) {
		arrangemon(m);
		restack(m);
	} else for (m = mons; m; m = m->next)
		       arrangemon(m);
}

void
sendmon(Client *c, Monitor *m)
{
	Monitor *mon_client = get_monitor_from_client(c);

	if (mon_client == m)
		return;
	unfocus(c, 1);
	detach(mon_client, c);
	detachstack(mon_client, c);
	c->tags = m->current_tag; /* assign tags of target monitor */
	attach(m, c);
	attachstack(m, c);
	focus(m, NULL);
	arrange(NULL);
}

void
movemouse(const Arg *arg)
{
	int x, y, ocx, ocy, nx, ny;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (c->isfullscreen) /* no support moving fullscreen windows by mouse */
		return;
	restack(selmon);
	ocx = c->x;
	ocy = c->y;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
			 None, get_cursor(CurMove), CurrentTime) != GrabSuccess)
		return;
	if (!getrootptr(&x, &y))
		return;
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handle_events(ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / 60))
				continue;
			lasttime = ev.xmotion.time;

			nx = ocx + (ev.xmotion.x - x);
			ny = ocy + (ev.xmotion.y - y);
			if (nx >= selmon->wx && nx <= selmon->wx + selmon->ww
			    && ny >= selmon->wy && ny <= selmon->wy + selmon->wh) {
				if (abs(selmon->wx - nx) < snap)
					nx = selmon->wx;
				else if (abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) < snap)
					nx = selmon->wx + selmon->ww - WIDTH(c);
				if (abs(selmon->wy - ny) < snap)
					ny = selmon->wy;
				else if (abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < snap)
					ny = selmon->wy + selmon->wh - HEIGHT(c);
				if (!c->isfloating && (selmon->current_layout != floating)
				    && (abs(nx - c->x) > snap || abs(ny - c->y) > snap))
					togglefloating(NULL);
			}
			if ((selmon->current_layout == floating) || c->isfloating)
				resize(c, nx, ny, c->w, c->h, 1);
			break;
		}
	} while (ev.type != ButtonRelease);
	XUngrabPointer(dpy, CurrentTime);
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(selmon, NULL);
	}
}

void
quit(const Arg *arg)
{
	running = 0;
}

void
resizemouse(const Arg *arg)
{
	int ocx, ocy, nw, nh;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (c->isfullscreen) /* no support resizing fullscreen windows by mouse */
		return;
	restack(selmon);
	ocx = c->x;
	ocy = c->y;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
			 None, get_cursor(CurResize), CurrentTime) != GrabSuccess)
		return;
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w - 1, c->h - 1);
	do {
		Monitor *m = get_monitor_from_client(c);
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handle_events(ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / 60))
				continue;
			lasttime = ev.xmotion.time;

			nw = MAX(ev.xmotion.x - ocx + 1, 1);
			nh = MAX(ev.xmotion.y - ocy + 1, 1);
			if (m->wx + nw >= selmon->wx && m->wx + nw <= selmon->wx + selmon->ww
			    && m->wy + nh >= selmon->wy && m->wy + nh <= selmon->wy + selmon->wh)
			{
				if (!c->isfloating && (selmon->current_layout != floating)
				    && (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
					togglefloating(NULL);
			}
			if ((selmon->current_layout == floating) || c->isfloating)
				resize(c, c->x, c->y, nw, nh, 1);
			break;
		}
	} while (ev.type != ButtonRelease);
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w - 1, c->h - 1);
	XUngrabPointer(dpy, CurrentTime);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(selmon, NULL);
	}
}

void
nextlayout(const Arg *arg)
{
	/* increment layout selected */
	selmon->current_layout = (selmon->current_layout + 1) % last_layout;

	/* copy symbol to selected monitor */
	strncpy(selmon->ltsymbol, get_layout(selmon->current_layout).symbol, sizeof selmon->ltsymbol);

	if (selmon->sel)
		arrange(selmon);
	else
		drawbar(selmon);
}

void
spawn(const Arg *arg)
{
	if (fork() == 0) {
		if (dpy)
			close(ConnectionNumber(dpy));
		setsid();
		execvp(((char **)arg->v)[0], (char **)arg->v);
		fprintf(stderr, "jwm: execvp %s", ((char **)arg->v)[0]);
		perror(" failed");
		exit(EXIT_SUCCESS);
	}
}

void
tag(const Arg *arg)
{
	if (selmon->sel) {
		selmon->sel->tags = arg->ui;
		focus(selmon, NULL);
		arrange(selmon);
	}
}

void
togglefloating(const Arg *arg)
{
	if (!selmon->sel)
		return;
	if (selmon->sel->isfullscreen) /* no support for fullscreen windows */
		return;
	selmon->sel->isfloating = !selmon->sel->isfloating || selmon->sel->isfixed;
	if (selmon->sel->isfloating)
		resize(selmon->sel, selmon->sel->x, selmon->sel->y,
		       selmon->sel->w, selmon->sel->h, 0);
	arrange(selmon);
}

void
view(const Arg *arg)
{
	selmon->current_tag = arg->ui;
	focus(selmon, NULL);
	arrange(selmon);
}

void
manage(Window w, XWindowAttributes *wa)
{
	Client *c, *t = NULL;
	Monitor *m = NULL;
	Window trans = None;
	XWindowChanges wc;
	int sw = get_screen()->width;
	int bh = get_bar_height();

	c = ecalloc(1, sizeof(Client));
	c->win = w;
	/* geometry */
	c->x = c->oldx = wa->x;
	c->y = c->oldy = wa->y;
	c->w = c->oldw = wa->width;
	c->h = c->oldh = wa->height;

	updatetitle(c);
	if (XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) {
		m = get_monitor_from_client(t);
		c->tags = t->tags;
	} else {
		m = selmon;
		c->tags = m->current_tag;
	}

	if (c->x + WIDTH(c) > m->mx + m->mw)
		c->x = m->mx + m->mw - WIDTH(c);
	if (c->y + HEIGHT(c) > m->my + m->mh)
		c->y = m->my + m->mh - HEIGHT(c);
	c->x = MAX(c->x, m->mx);
	/* only fix client y-offset, if the client center might cover the bar */
	c->y = MAX(c->y, ((m->by == m->my) && (c->x + (c->w / 2) >= m->wx)
			  && (c->x + (c->w / 2) < m->wx + m->ww)) ? bh : m->my);

	wc.border_width = 0; /* no border */
	XConfigureWindow(dpy, w, CWBorderWidth, &wc);
	XSetWindowBorder(dpy, w, get_scheme_pixel(SchemeNorm));
	configure(c); /* propagates border_width, if size doesn't change */
	updatewindowtype(c);
	updatesizehints(c);
	updatewmhints(c);
	XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
	grabbuttons(c, 0);
	if (!c->isfloating)
		c->isfloating = c->oldstate = trans != None || c->isfixed;
	if (c->isfloating)
		XRaiseWindow(dpy, c->win);
	attach(m, c);
	attachstack(m, c);
	XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
	                (unsigned char *) &(c->win), 1);
	XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w, c->h); /* some windows require this */
	setclientstate(c, NormalState);
	if (m == selmon)
		unfocus(selmon->sel, 0);
	m->sel = c;
	arrange(m);
	XMapWindow(dpy, c->win);
	focus(m, NULL);
}

void
unmanage(Client *c, int destroyed)
{
	Monitor *m = get_monitor_from_client(c);
	XWindowChanges wc;

	/* The server grab construct avoids race conditions. */
	detach(m, c);
	detachstack(m, c);
	if (!destroyed) {
		wc.border_width = 0; /* no border */
		XGrabServer(dpy);
		XSetErrorHandler(xerrordummy);
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc); /* restore border */
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		setclientstate(c, WithdrawnState);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
	free(c);
	focus(m, NULL);
	updateclientlist();
	arrange(m);
}

int
running_state(void)
{
	return running;
}
