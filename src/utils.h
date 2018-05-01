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

#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdint.h>

#define LENGTH(x)       (sizeof(x) / sizeof(*x))

#define DO_PRAGMA(x) _Pragma (#x)
#define TODO(x) DO_PRAGMA(message ("TODO: " #x))

/* file utils */
bool file_access(const char *pathname);
bool file_write(const char *pathname, const void *buf, size_t count);
bool file_read(const char *pathname, void *buf, size_t count);

/* pattern matching */
bool regex_match(const char *string,  const char *regex);
bool regex_extract(const char* string,  const char *regex,
		   size_t nmatch, char pmatch[][256]);

/* system utils */
void get_process_name(uint32_t pid, char *name, size_t len);

#endif
