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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "utils.h"
#include "log.h"

bool file_access(const char *pathname)
{
	return access(pathname, F_OK | R_OK) ? false : true;
}

bool file_read(const char *pathname, void *buf, size_t count)
{
	int fd = -1;

	fd = open(pathname, O_RDONLY);
	if (fd == -1) {
		LOGE("%s", strerror(errno));
		return false;
	}

	if (read(fd, buf, count) == -1) {
		LOGE("%s", strerror(errno));
		return false;
	}

	if (close(fd) == -1) {
		LOGE("%s", strerror(errno));
		return false;
	}

	return true;
}

bool file_write(const char *pathname, const void *buf, size_t count)
{
	int fd = -1;

	fd = open(pathname, O_WRONLY);
	if (fd == -1) {
		LOGE("%s", strerror(errno));
		return false;
	}

	if (write(fd, buf, count) == -1) {
		LOGE("%s", strerror(errno));
		return false;
	}

	if (close(fd) == -1) {
		LOGE("%s", strerror(errno));
		return false;
	}

	return true;
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
