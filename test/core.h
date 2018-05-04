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

#ifndef CORE_H
#define CORE_H

#include <check.h>

#define CORE_TEST_START(test) \
	START_TEST(test) \

#define CORE_TEST_END(test) \
	END_TEST \
	static void __attribute__((constructor(210))) init_test_##test(void) \
	{ \
		register_test(__FILE__, test, #test); \
	} \

void register_test(char *tcase, TFun fn, char *test_name);

#endif
