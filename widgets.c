#include <string.h>
#include <stdio.h>

#include "widgets.h"
#include "util.h"

/* extern widgets functions */
extern char *date(void);
extern char *ram(void);
extern char *cpu(void);
extern char *battery(void);

/* Static vars */
static const char separator[] = "      ";
static char* (*widgets[])(void) = {
	cpu,
	ram,
	battery,
	date
};

void
get_widgets(char *text, size_t size)
{
	int i, n = 0;

	/* reset text */
	memset(text, '\0', size);

	/* add widgets */
	for (i = 0; i < LENGTH(widgets); i++)
	{
		char *widget = widgets[i]();

		if (strlen(widget) + strlen(separator) + strlen(text) < size) {
			sprintf(text + n, "%s", widget);
			n = n + strlen(widget);

			/* don't add separator on last element */
			if ( i < LENGTH(widgets) - 1) {
				sprintf(text + n, "%s", separator);
				n = n + strlen(separator);
			}
		} else
			break;
	}
}
