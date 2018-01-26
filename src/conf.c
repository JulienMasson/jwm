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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "log.h"
#include "conf.h"
#include "utils.h"

/* global vars */
struct conf global_conf;
static char *conf_path;

static void parse_line(char *line, ssize_t nread)
{
	char key[nread], value[nread];
	static char log_file[256];
	static char wallpaper[256];

	memset(key, '\0', nread);
	memset(value, '\0', nread);

	/* key and value successfully matched and assigned */
	if (sscanf(line, "%[^=]=%s", &key, &value) == 2)
		if (strncmp(key, "log_level", nread) == 0)
			global_conf.log_level = atoi(value);
		else if (strncmp(key, "log_file", nread) == 0) {
			memset(log_file, '\0', 256);
			snprintf(log_file, 256, "%s", value);
			global_conf.log_file = log_file;
		} else if (strncmp(key, "wallpaper", nread) == 0) {
			memset(wallpaper, '\0', 256);
			snprintf(wallpaper, 256, "%s", value);
			global_conf.wallpaper = wallpaper;
		}
}

int conf_read(void)
{
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;

	fp = fopen(conf_path, "r");
	if (fp == NULL) {
		LOGE("%s", strerror(errno));
		return -1;
	}

	while ((nread = getline(&line, &len, fp)) != -1)
		parse_line(line, nread);

	fclose(fp);
	if (line)
		free(line);

	return 0;
}

void conf_init(char *path)
{
	static char conf_path_default[256];

	/* conf file exist and readable */
	if (path && (file_access(path) == true))
		conf_path = path;
	else {
		/* default conf file */
		memset(conf_path_default, '\0', 256);
		snprintf(conf_path_default, 256, "%s/.jwmrc", getenv("HOME"));
		conf_path = conf_path_default;
	}

	/* default value of global conf */
	global_conf.log_level = LOG_WARN;
	global_conf.log_file = NULL;
	global_conf.wallpaper = NULL;

	/* read config */
	conf_read();
}
