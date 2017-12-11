/*
 * This file is part of the jwm distribution:
 * https://github.com/JulienMasson/jwm
 *
 * Copyright (c) 2017 Julien Masson.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include "log.h"
#include "conf.h"

#define LOG_FILE "/home/lab/.config/jwm/log"

static int log_level;
static const char *level_names[] = {
	"ERROR", "WARN", "INFO", "DEBUG"
};
static const char *level_colors[] = {
	"\x1b[31m", "\x1b[33m", "\x1b[34m", "\x1b[32m"
};

void log_init(void)
{
	/* redirect stdout */
        freopen(LOG_FILE, "a", stdout);
	setbuf(stdout, NULL);

	/* redirect stderr */
        freopen(LOG_FILE, "a", stderr);
	setbuf(stderr, NULL);

	/* set log level from global conf */
	log_level = global_conf.log_level;
}

void log_set_level(int level)
{
	log_level = level;
}

void log_log(int level, const char *file, int line, const char *fmt, ...)
{
	time_t t;
	struct tm *lt;
	va_list args;
	char buf[16];

	if (level <= log_level) {
		 /* shift level to select right colors/name */
		level = level + 1;

		/* Get current time */
		t = time(NULL);
		lt = localtime(&t);
		buf[strftime(buf, sizeof(buf), "%H:%M:%S", lt)] = '\0';

		/* print header: date level file line */
		fprintf(stderr,
			"%s %s%-5s\x1b[0m \x1b[33m%s:%-4d\x1b[0m ",
			buf, level_colors[level], level_names[level], file, line);

		/* print args */
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);
		fprintf(stderr, "\n");
	}
}
