/*
 * This file is part of the jwm distribution:
 * https://github.com/JulienMasson/jwm
 *
 * Copyright (c) 2018 Julien Masson.
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
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "utils.h"
#include "log.h"

bool file_access(const char *pathname)
{
	return access(pathname, F_OK | R_OK) ? false : true;
}

void get_process_name(uint32_t pid, char *name, size_t len)
{
	FILE *fp;
	char proc_path[BUFSIZ];
	size_t size;

	snprintf(proc_path, BUFSIZ, "/proc/%d/cmdline", pid);
	fp = fopen(proc_path, "r");
	if (fp == NULL) {
		LOGE("%s", strerror(errno));
		return;
	}

	size = fread(name, sizeof(char), len, fp);
	if ((size > 0) && (name[size - 1] == '\n'))
		name[size - 1] = '\0';

	fclose(fp);
}
