#ifndef EVENT_H
#define EVENT_H

#include <X11/Xlib.h>

#include "jwm.h"

void setup_events(void);
void handle_events(XEvent ev);
int sendevent(Client *c, Atom proto);

#endif
