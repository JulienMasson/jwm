#ifndef JWM_H
#define JWM_H

#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>

/* macros */
#define WIDTH(X)                ((X)->w)
#define HEIGHT(X)               ((X)->h)
#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)

typedef union {
	int i;
	unsigned int ui;
	float f;
	const void *v;
} Arg;

#endif
