#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *
date(void)
{
	static char date[17];
	time_t t;

	memset(date, '\0', sizeof(date));

	t = time(NULL);
	snprintf(date, sizeof(date), "%s", ctime(&t));

	return date;
}
