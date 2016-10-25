#ifndef KEY_H
#define KEY_H

#include "jwm.h"

void grabkeys(void);
void grabbuttons(Client *c, int focused);
void handle_key_events(XKeyEvent *ev);
void handle_button_events(XButtonPressedEvent *ev, unsigned int click, Arg arg);

#endif
