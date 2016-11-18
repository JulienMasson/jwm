#include "font.h"
#include "extern.h"
#include "util.h"

/* Static vars */
static const char *fonts[1] = { "Ubuntu:size=9" };

void
setup_font(void)
{
	if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
		die("no fonts could be loaded.");
}
