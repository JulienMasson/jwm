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
#include <regex.h>

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

bool regex_match(const char *string,  const char *regex)
{
	regex_t preg;
	int ret;
	char error_msg[256];
	memset(error_msg, '\0', 256);

	/* compile regular expression */
	ret = regcomp(&preg, regex, REG_NOSUB | REG_EXTENDED);
	if (ret != 0) {
		regerror(ret, &preg, error_msg, 256);
		LOGE("%s\n", error_msg);
		return false;
	}

	/* execute regular expression */
	ret = regexec(&preg, string, 0, NULL, 0);
	regfree(&preg);

	/* check the return value */
	if (ret == 0)
		return true;
	else if (ret == REG_NOMATCH)
		return false;
	else {
		regerror(ret, &preg, error_msg, 256);
		LOGE("%s\n", error_msg);
		return false;
	}
}

bool regex_extract(const char* string,  const char *regex,
		   size_t nmatch, char pmatch[][256])
{
	regex_t preg;
	uint8_t ret, i;
	char error_msg[256];
	regmatch_t matches[nmatch + 1];
	regoff_t so, eo;
	memset(error_msg, '\0', 256);

	/* compile regular expression */
	ret = regcomp(&preg, regex, REG_EXTENDED);
	if (ret != 0) {
		regerror(ret, &preg, error_msg, 256);
		LOGE("%s\n", error_msg);
		return false;
	}

	/* execute regular expression */
	ret = regexec(&preg, string, nmatch + 1, matches, 0);
	regfree(&preg);

	/* check the return value */
	if (ret == 0) {
		for (i = 0; i < nmatch; i++) {
			so = matches[i + 1].rm_so;
			eo = matches[i + 1].rm_eo;
			if (so != -1 && eo != -1)
				memcpy(pmatch[i], string + so, eo - so);
		}
		return true;
	} else if (ret == REG_NOMATCH)
		return false;
	else {
		regerror(ret, &preg, error_msg, 256);
		LOGE("%s\n", error_msg);
		return false;
	}
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
