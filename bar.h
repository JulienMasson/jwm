#ifndef BAR_H
#define BAR_H

#include "jwm.h"
#include "window.h"

typedef enum {
	SchemeNorm,
	SchemeSel,
	SchemeLast
} scheme_t;

void init_bars_properties(void);
void setup_bars(void);
void drawbar(Monitor *m);
void drawbars(void);
void updatebarpos(Monitor *m);
void updatebars(void);
void updatestatus(void);
unsigned int text_width(const char *text);
void updatebarpos(Monitor *m);
int get_bar_height(void);
int get_bar_width(void);
unsigned long get_scheme_pixel(scheme_t type);
void cleanup_bar(void);

#endif
