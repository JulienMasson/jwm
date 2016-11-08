#include "layout.h"
#include "util.h"
#include "extern.h"


/* static declaration functions */
static void float_layout(Monitor *m);
static void tile_layout(Monitor *m);
static void tab_layout(Monitor *m);

/* Global vars */
static Layout layouts[last_layout] = {
	/* symbol     arrange function */
	{ "[ F ]",    float_layout },
	{ "[ T ]",    tile_layout  },
	{ "[ M ]",    tab_layout   }
};


Client *
nexttiled(Client *c)
{
	Monitor *m = get_monitor_from_client(c);
	for (; c && (c->isfloating || !ISVISIBLE(c,m)); c = c->next);
	return c;
}

void
float_layout(Monitor *m)
{
}

void
tile_layout(Monitor *m)
{
	unsigned int i, n, h, mw, ty;
	Client *c;

	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
	if (n == 0)
		return;

	/* windows split by 2 */
	if ( n > 1)
		mw = m->window.width / 2;
	else
		mw = m->window.width;

	/* master client */
	c = nexttiled(m->clients);
	resize(c, m->window.x, m->window.y, mw, m->window.height, 0);
	c = nexttiled(c->next);

	/* tiled client */
	if (c) {
		for (i = 1, ty = 0; c; c = nexttiled(c->next), i++) {
			h = (m->window.height - ty) / (n - i);
			resize(c, m->window.x + mw, m->window.y + ty, m->window.width - mw, h, 0);
			ty += HEIGHT(c);
		}
	}
}

void
tab_layout(Monitor *m)
{
	Client *c;
	for (c = nexttiled(m->clients); c; c = nexttiled(c->next))
		resize(c, m->window.x, m->window.y, m->window.width, m->window.height, 0);
}

void
layout_configure_client(XConfigureRequestEvent *ev, Client *c)
{
	if (c->isfloating || (selmon->current_layout == floating)) {
		Monitor *m = get_monitor_from_client(c);
		if (ev->value_mask & CWX) {
			c->oldx = c->x;
			c->x = m->screen.x + ev->x;
		}
		if (ev->value_mask & CWY) {
			c->oldy = c->y;
			c->y = m->screen.y + ev->y;
		}
		if (ev->value_mask & CWWidth) {
			c->oldw = c->w;
			c->w = ev->width;
		}
		if (ev->value_mask & CWHeight) {
			c->oldh = c->h;
			c->h = ev->height;
		}
		if ((c->x + c->w) > m->screen.x + m->screen.width && c->isfloating)
			c->x = m->screen.x + (m->screen.width / 2 - WIDTH(c) / 2); /* center in x direction */
		if ((c->y + c->h) > m->screen.y + m->screen.height && c->isfloating)
			c->y = m->screen.y + (m->screen.height / 2 - HEIGHT(c) / 2); /* center in y direction */
		if ((ev->value_mask & (CWX|CWY)) && !(ev->value_mask & (CWWidth|CWHeight)))
			configure(c);
		if (ISVISIBLE(c, m))
			XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
	} else
		configure(c);
}

Layout
get_layout(layout_t type)
{
	return layouts[type];
}
