#ifndef BAR_H
#define BAR_H

#include "jwm.h"

void init_bars_properties(void);
void setup_bars(void);
void drawbar(Monitor *m);
void drawbars(void);
void updatebarpos(Monitor *m);
void updatebars(void);
void updatestatus(void);

#endif
