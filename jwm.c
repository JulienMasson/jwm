/* See LICENSE file for copyright and license details.
 *
 * dynamic window manager is designed like any other X client as well. It is
 * driven through handling X events. In contrast to other X clients, a window
 * manager selects for SubstructureRedirectMask on the root window, to receive
 * events about window (dis-)appearance.  Only one X connection at a time is
 * allowed to select for this event mask.
 *
 * The event handlers of dwm are organized in an array which is accessed
 * whenever a new event has been fetched. This allows event dispatching
 * in O(1) time.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag.  Clients are organized in a linked client
 * list on each monitor, the focus history is remembered through a stack list
 * on each monitor. Each client contains a bit array to indicate the tags of a
 * client.
 *
 * Keys and tagging rules are organized as arrays and defined in config.h.
 *
 * To understand everything else, start reading main().
 */
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "bar.h"
#include "drw.h"
#include "util.h"
#include "window.h"
#include "atom.h"
#include "key.h"
#include "event.h"
#include "screen.h"
#include "client.h"

/* variables */
const char broken[] = "broken";
int screen;
int sw, sh;           /* X display screen geometry width, height */
int bh, blw = 0;      /* bar geometry */
int lrpad;            /* sum of left and right padding for text */
unsigned int numlockmask = 0;
Atom wmatom[WMLast], netatom[NetLast];
int running = 1;
Cur *cursor[CurLast];
Scm scheme[SchemeLast];
Display *dpy;
Drw *drw;
Monitor *mons, *selmon;
Window root;

/* function implementations */
void
cleanup(void)
{
	Arg a = {.ui = ~0};
	Layout foo = { "", NULL };
	Monitor *m;
	size_t i;

	view(&a);
	selmon->lt[selmon->sellt] = &foo;
	for (m = mons; m; m = m->next)
		while (m->stack)
			unmanage(m->stack, 0);
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	while (mons)
		cleanupmon(mons);
	for (i = 0; i < CurLast; i++)
		drw_cur_free(drw, cursor[i]);
	for (i = 0; i < SchemeLast; i++)
		free(scheme[i]);
	drw_free(drw);
	XSync(dpy, False);
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
}

void
run(void)
{
	XEvent ev;
	int x11_fd = ConnectionNumber(dpy);
	fd_set in_fds;
	struct timeval tv;

	// Main loop
	while(1) {
		// Create a File Description Set containing x11_fd
		FD_ZERO(&in_fds);
		FD_SET(x11_fd, &in_fds);

		// Set our timer.  One second sounds good.
		tv.tv_usec = 0;
		tv.tv_sec = 1;

		// Wait for X Event or a Timer
		int num_ready_fds = select(x11_fd + 1, &in_fds, NULL, NULL, &tv);

		// Timer Fired
		if (num_ready_fds == 0)
			drawbars();

		// Handle XEvents and flush the input
		while(XPending(dpy)) {
			XNextEvent(dpy, &ev);
			handle_events(ev);
		}

		if (!running)
			break;
	}
}

void
scan(void)
{
	unsigned int i, num;
	Window d1, d2, *wins = NULL;
	XWindowAttributes wa;

	if (XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
		for (i = 0; i < num; i++) {
			if (!XGetWindowAttributes(dpy, wins[i], &wa)
			    || wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
				continue;
			if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
				manage(wins[i], &wa);
		}
		for (i = 0; i < num; i++) { /* now the transients */
			if (!XGetWindowAttributes(dpy, wins[i], &wa))
				continue;
			if (XGetTransientForHint(dpy, wins[i], &d1)
			    && (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
				manage(wins[i], &wa);
		}
		if (wins)
			XFree(wins);
	}
}

void
sigchld(int unused)
{
	if (signal(SIGCHLD, sigchld) == SIG_ERR)
		die("can't install SIGCHLD handler:");
	while (0 < waitpid(-1, NULL, WNOHANG));
}

void
setup()
{
	/* clean up any zombies immediately */
	sigchld(0);

	/* setup screen */
	setup_screen();

	/* init bars properties */
	init_bars_properties();

	/* update geometry */
	updategeom();

	/* setup atoms */
	setup_atoms();

	/* setup bars */
	setup_bars();

	/* EWMH support per view */
	support_ewmh();

	/* setup events */
	setup_events();

	/* setup key */
	grabkeys();

	/* focus main screen */
	focus(NULL);
}

int
main(int argc, char *argv[])
{
	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("warning: no locale support\n", stderr);
	if (!(dpy = XOpenDisplay(NULL)))
		die("jwm: cannot open display");

	/* log file */
	freopen(".jwm.log", "a", stdout); setbuf(stdout, NULL);
	freopen(".jwm.log", "a", stderr); setbuf(stderr, NULL);

	setup();

	scan();

	run();

	cleanup();
	XCloseDisplay(dpy);
	return EXIT_SUCCESS;
}
