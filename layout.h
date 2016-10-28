#ifndef LAYOUT_H
#define LAYOUT_H

#include "window.h"

void layout_configure_client(XConfigureRequestEvent *ev, Client *c);
Layout get_layout(layout_t type);

#endif
