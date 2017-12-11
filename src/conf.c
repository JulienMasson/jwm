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
#include "log.h"
#include "conf.h"

#define CONF_FILE "/home/lab/.config/jwm/jwmrc"

struct conf global_conf;

static void parse_line(char *line, ssize_t nread)
{
	char key[nread], value[nread];

	memset(key, '\0', nread);
	memset(value, '\0', nread);

	sscanf(line, "%[^=]=%s", &key, &value);

	if (strlen(value) > 0)
		if (strncmp(key, "log_level", nread) == 0) {
			int log_level = atoi(value);
			global_conf.log_level = log_level;
		}
}

void conf_init(void)
{
	/* default conf */
	global_conf.log_level = LOG_WARN;

	if (conf_read() == -1)
		LOGE("Fail to read conf");
}

int conf_read(void)
{
        FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;

        if ((fp = fopen(CONF_FILE, "r")) == NULL) {
		LOGE("%s", strerror(errno));
		return -1;
	}

	while ((nread = getline(&line, &len, fp)) != -1) {
		parse_line(line, nread);
	}

	fclose(fp);
	if (line)
		free(line);

	return 0;
}
