#ifndef CLIENT_H
#define CLIENT_H

#include "drw.h"
#include "jwm.h"

void focus(Client *c);
void unfocus(Client *c, int setfocus);
void setfocus(Client *c);
void resize(Client *c, int x, int y, int w, int h, int interact);
void resizeclient(Client *c, int x, int y, int w, int h);
void attach(Client *c);
void detach(Client *c);
void attachstack(Client *c);
void detachstack(Client *c);
void showhide(Client *c);
void setfullscreen(Client *c, int fullscreen);
void pop(Client *c);
void configure(Client *c);
void updatesizehints(Client *c);
void updatewmhints(Client *c);
void updatetitle(Client *c);
void updatewindowtype(Client *c);
void setclientstate(Client *c, long state);
void updateclientlist(void);

#endif
