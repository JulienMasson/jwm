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
#include "font.h"

/* Global vars */
Atom wmatom[WMLast], netatom[NetLast];
Display *dpy;
Drw *drw;
Monitor *mons, *selmon;
Window root;

/* function implementations */
void
cleanup(void)
{
	Arg a = {.ui = ~0};
	Monitor *m;

	view(&a);
	for (m = mons; m; m = m->next)
		while (m->stack)
			unmanage(m->stack, 0);
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	while (mons)
		cleanupmon(mons);
	cleanup_cursors();
	cleanup_bar();
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

	/* Main loop */
	while(running_state()) {

		/* Create a File Description Set containing x11_fd */
		FD_ZERO(&in_fds);
		FD_SET(x11_fd, &in_fds);

		/* Set our timer to One second */
		tv.tv_usec = 0;
		tv.tv_sec = 1;

		/* Wait for X Event or a Timer */
		int num_ready_fds = select(x11_fd + 1, &in_fds, NULL, NULL, &tv);

		/* Timer Fired */
		if (num_ready_fds == 0)
			drawbars();

		/* Handle XEvents and flush the input */
		while(XPending(dpy)) {
			XNextEvent(dpy, &ev);
			handle_events(ev);
		}

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

	/* setup font */
	setup_font();

	/* init cursors */
	init_cursors();

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
	focus(selmon, NULL);
}

int
main(int argc, char *argv[])
{
	/* Locale support */
	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("warning: no locale support\n", stderr);

	/* open display */
	if (!(dpy = XOpenDisplay(NULL)))
		die("jwm: cannot open display");

	/* log file */
	if (freopen(".jwm.log", "a", stdout))
		setbuf(stdout, NULL);
	if (freopen(".jwm.log", "a", stderr))
		setbuf(stderr, NULL);

	/* setup all features */
	setup();

	/* scan window tree information */
	scan();

	/* main loop */
	run();

	/* cleanup all features */
	cleanup();

	/* close display */
	XCloseDisplay(dpy);

	return EXIT_SUCCESS;
}
