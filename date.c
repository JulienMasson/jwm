#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *
date(void)
{
	static char date[19];
	struct tm *tm;
	time_t ts;

	memset(date, '\0', sizeof(date));
	if ((ts = time(NULL)) == ((time_t) - 1)) {
		fprintf(stderr, "time failed");
		goto end;
	}
	if (!(tm = localtime(&ts))) {
		fprintf(stderr, "localtime failed");
		goto end;
	}
	if (!strftime(date, sizeof(date), "%a %b %d,  %H:%M", tm))
		fprintf(stderr, "strftime failed");
end:
	return date;
}
