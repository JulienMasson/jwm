#ifndef CLIENT_H
#define CLIENT_H

#include "drw.h"
#include "jwm.h"

typedef struct Client Client;
struct Client {
	char name[256];
	float mina, maxa;
	int x, y, w, h;
	int oldx, oldy, oldw, oldh;
	int basew, baseh, incw, inch, maxw, maxh, minw, minh;
	int bw, oldbw;
	unsigned int tags;
	int isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen;
	Client *next;
	Client *snext;
	Window win;
};

void clearurgent(Client *c);
void unfocus(Client *c, int setfocus);
void setfocus(Client *c);
void resize(Client *c, int x, int y, int w, int h, int interact);
void resizeclient(Client *c, int x, int y, int w, int h);
void configure(Client *c);
void updatesizehints(Client *c);
void updatewmhints(Client *c);
void updatetitle(Client *c);
void updatewindowtype(Client *c);
void setclientstate(Client *c, long state);

#endif
