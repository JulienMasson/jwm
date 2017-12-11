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

#ifndef LOG_H
#define LOG_H

enum { LOG_NOTHING, LOG_ERROR, LOG_WARN, LOG_INFO, LOG_DEBUG };

#define LOGE(...) log_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOGW(...) log_log(LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define LOGI(...) log_log(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOGD(...) log_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)

#define TRACE() log_log(LOG_DEBUG, __FILE__, __LINE__, "%s", __FUNCTION__)

void log_init(void);

void log_set_level(int level);

void log_log(int level, const char *file, int line, const char *fmt, ...);

#endif
