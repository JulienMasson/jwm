#ifndef KEY_H
#define KEY_H

#include "jwm.h"
#include "client.h"

typedef enum {
	ClkTagBar,
	ClkClientWin,
	ClkRootWin,
	ClkLast
} click_t;

void grabkeys(void);
void grabbuttons(Client *c, int focused);
void handle_key_events(XKeyEvent *ev);
void handle_button_events(XButtonPressedEvent *ev, click_t click, Arg arg);

#endif
