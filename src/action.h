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

#ifndef ACTION_H
#define ACTION_H

#include "types.h"

void focusnext(const Arg *arg);
void maxhalf(const Arg *arg);
void changescreen(const Arg *arg);
void deletewin(const Arg *arg);
void maximize(const Arg *arg);
void hide(const Arg *arg);
void raise_all(const Arg *arg);
void start(const Arg *arg);
void jwm_exit(const Arg *arg);
void mousemotion(const Arg *arg);

#endif
