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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "log.h"
#include "conf.h"
#include "utils.h"

/* global vars */
static const char * const level_names[] = {
	"ERROR", "WARN", "INFO", "DEBUG"
};
static const char * const level_colors[] = {
	"\x1b[31m", "\x1b[33m", "\x1b[34m", "\x1b[32m"
};

void log_init(void)
{
	static char log_file_default[256];

	/* if file doesn't exist, set to default */
	if (file_access(global_conf.log_file) == false) {
		memset(log_file_default, '\0', 256);
		snprintf(log_file_default, 256, "%s/.jwm.log", getenv("HOME"));
		global_conf.log_file = log_file_default;
	}

	/* redirect stdout */
	freopen(global_conf.log_file, "a", stdout);
	setbuf(stdout, NULL);

	/* redirect stderr */
	freopen(global_conf.log_file, "a", stderr);
	setbuf(stderr, NULL);
}

void log_log(int level, const char *file, int line, const char *fmt, ...)
{
	time_t t;
	struct tm *lt;
	va_list args;
	char date[16];

	if (level <= global_conf.log_level) {
		 /* shift level to select right colors/name */
		level = level - 1;

		/* Get current date */
		t = time(NULL);
		lt = localtime(&t);
		date[strftime(date, sizeof(date), "%H:%M:%S", lt)] = '\0';

		/* print header: date level file line */
		printf("%s %s%-5s\x1b[0m \x1b[33m%s:%-4d\x1b[0m",
		       date,
		       level_colors[level], level_names[level],
		       file, line);

		/* print args */
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);
		printf("\n");
	}
}
