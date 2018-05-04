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
#include <stdlib.h>
#include <stdbool.h>
#include <libgen.h>

#include "core.h"

struct test_unit {
	TFun test;
	struct test_unit *next;
};

struct test_case {
	char *name;
	TCase *tc;
	struct test_unit *tests;
	struct test_case *next;
};

static struct test_case *tcases;

static bool found_test(TFun test)
{
	struct test_case *tcase;
	struct test_unit *tunit;

	for (tcase = tcases; tcase != NULL; tcase = tcase->next)
		for (tunit = tcase->tests; tunit != NULL; tunit = tunit->next)
			if (test == tunit->test)
				return true;
	return false;
}

static struct test_case* get_test_case(char *name)
{
	struct test_case *tcase;

	for (tcase = tcases; tcase != NULL; tcase = tcase->next)
		if (strncmp(tcase->name, name, 256) == 0)
			return tcase;
	return NULL;
}

static struct test_case* create_test_case(char *tcase_name)
{
	struct test_case *tcase;
	char *name, *extension;

	tcase = malloc(sizeof(struct test_case));
	if (tcase == NULL)
		return NULL;

	name = malloc(sizeof(char) * 256);
	memset(name, '\0', 256);
	memcpy(name, tcase_name, 256);

	/* take basename and remove extension */
	tcase->name = basename(name);
	extension = strstr(tcase->name, ".");
	if (extension)
		*extension = '\0';

	tcase->tc = tcase_create(tcase->name);
	tcase->next = NULL;

	return tcase;
}

static void add_test_case(struct test_case *tcase)
{
	if (tcases == NULL)
		tcases = tcase;
	else {
		tcase->next = tcases;
		tcases = tcase;
	}
}

static struct test_unit* create_test_unit(TFun test)
{
	struct test_unit *tunit;
	tunit = malloc(sizeof(struct test_case));
	if (tunit == NULL)
		return NULL;

	tunit->test = test;
	tunit->next = NULL;

	return tunit;
}

static void add_test_unit(struct test_case *tcase, struct test_unit *tunit)
{
	if (tcase->tests == NULL)
		tcase->tests = tunit;
	else {
		tunit->next = tcase->tests;
		tcase->tests = tunit;
	}
}

void register_test(char *tcase_name, TFun test)
{
	struct test_case *tcase;
	struct test_unit *tunit;

	if (found_test(test) == false) {

		/* get test case or create/add it if not found */
		tcase = get_test_case(tcase_name);
		if (tcase == NULL) {
			tcase = create_test_case(tcase_name);
			if (tcase == NULL)
				return;
			add_test_case(tcase);
		}

		/* create/add test unit to the test case */
		tunit = create_test_unit(test);
		if (tunit == NULL)
			return;
		add_test_unit(tcase, tunit);
	}
}

static void init_test(Suite *suite)
{
	struct test_case *tcase;
	struct test_unit *tunit;

	for (tcase = tcases; tcase != NULL; tcase = tcase->next) {

		suite_add_tcase(suite, tcase->tc);

		for (tunit = tcase->tests; tunit != NULL; tunit = tunit->next)
			tcase_add_test(tcase->tc, tunit->test);
	}
}

int main(void)
{
	Suite *suite = suite_create("Test");
	SRunner *sr = srunner_create(suite);
	int nf;

	init_test(suite);

	srunner_run_all(sr, CK_ENV);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);

	return nf == 0 ? 0 : 1;
}
