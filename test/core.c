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
#include <unistd.h>

#include "core.h"

#define FAIL(msg) "\x1b[31m" #msg "\x1b[0m"
#define PASS(msg) "\x1b[32m" #msg "\x1b[0m"

struct test_unit {
	TFun test;
	char *name;
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

static struct test_case *get_test_case(char *name)
{
	struct test_case *tcase;

	for (tcase = tcases; tcase != NULL; tcase = tcase->next)
		if (strncmp(tcase->name, name, 256) == 0)
			return tcase;
	return NULL;
}

static struct test_case *create_test_case(char *tcase_name)
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
	struct test_case *last;

	if (tcases == NULL)
		tcases = tcase;
	else {
		for (last = tcases; last->next != NULL; last = last->next)
			;
		last->next = tcase;
	}
}

static struct test_unit *create_test_unit(TFun test, char *test_name)
{
	struct test_unit *tunit;
	char *name;

	tunit = malloc(sizeof(struct test_case));
	if (tunit == NULL)
		return NULL;

	name = malloc(sizeof(char) * 256);
	memset(name, '\0', 256);
	memcpy(name, test_name, 256);

	tunit->test = test;
	tunit->name = name;
	tunit->next = NULL;

	return tunit;
}

static void add_test_unit(struct test_case *tcase, struct test_unit *tunit)
{
	struct test_unit *last;

	if (tcase->tests == NULL)
		tcase->tests = tunit;
	else {
		for (last = tcase->tests; last->next != NULL; last = last->next)
			;
		last->next = tunit;
	}
}

static char **get_all_test_name(void)
{
	struct test_case *tcase;
	struct test_unit *tunit;
	static char **test_names = NULL;
	int count = 0;
	char *name;

	if (test_names)
		return test_names;

	for (tcase = tcases; tcase != NULL; tcase = tcase->next) {

		for (tunit = tcase->tests; tunit != NULL; tunit = tunit->next) {

			name = malloc(sizeof(char) * 256);
			memset(name, '\0', 256);
			memcpy(name, tunit->name, 256);

			test_names = realloc(test_names, sizeof(char) * 256);
			test_names[count] = name;
			count++;
		}
	}

	return test_names;
}

void register_test(char *tcase_name, TFun test, char *test_name)
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
		tunit = create_test_unit(test, test_name);
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

static bool parse_results(SRunner *sr)
{
	TestResult **results;
	char **test_names;
	enum test_result result;
	int i, ntests;
	int pass = 0, failure = 0, error = 0;
	char tcase_name[256];

	/* get all tests results
	 * we make the assumption that all the results are
	 * classified in the same order that test_names
	 */
	results = srunner_results(sr);
	ntests = srunner_ntests_run(sr);
	test_names = get_all_test_name();

	/* print each test result */
	fprintf(stdout, "%-15s%-50s%s\n\n", "TEST CASE", "TEST UNIT", "RESULT");
	for (i = 0; i < ntests; i++) {

		/* print test case and test unit name */
		memset(tcase_name, '\0', 256);
		snprintf(tcase_name, 256, "%s:", tr_tcname(results[i]));
		fprintf(stdout, "%-15s", tcase_name);
		fprintf(stdout, "%-50s", test_names[i]);

		/* print result of the test */
		result = tr_rtype(results[i]);
		switch (result) {
		case CK_TEST_RESULT_INVALID:
			fprintf(stdout, FAIL(TEST RESULT INVALID));
			break;
		case CK_PASS:
			pass++;
			fprintf(stdout, PASS(PASS));
			break;
		case CK_FAILURE:
			failure++;
			fprintf(stdout, FAIL(FAILURE));
			break;
		case CK_ERROR:
			error++;
			fprintf(stdout, FAIL(ERROR));
			break;
		}

		/* if test failed print files, lines and error message */
		if (result != CK_PASS)
			fprintf(stdout, "\n%s:%d: %s\n", tr_lfile(results[i]),
				tr_lno(results[i]), tr_msg(results[i]));

		fprintf(stdout, "\n");
	}

	/* print summary */
	fprintf(stdout, "\n%d%%:  Pass:%d  Failures:%d  Errors:%d",
		(pass * 100) / (pass + failure + error), pass, failure, error);

	return (failure | error) == 0 ? true : false;
}

static void run_test(SRunner *sr)
{
	int stdout_saved = dup(STDOUT_FILENO);
	int stderr_saved = dup(STDERR_FILENO);

	/* silent stdio */
	freopen("/dev/null", "a", stdout);
	setbuf(stdout, NULL);
	freopen("/dev/null", "a", stderr);
	setbuf(stderr, NULL);

	/* run all test */
	srunner_run_all(sr, CK_SILENT);

	/* restore stdio */
	fclose(stdout);
	stdout = fdopen(stdout_saved, "w");
	setbuf(stdout, NULL);
	fflush(stderr);
	fclose(stderr);
	stderr = fdopen(stderr_saved, "w");
}

int main(void)
{
	Suite *suite;
	SRunner *sr;
	bool pass;

	/* create and init test suite */
	suite = suite_create("Test");
	sr = srunner_create(suite);
	init_test(suite);

	/* run test suite and parse result */
	fprintf(stdout,
		"\n============================================================"
		"============\n");
	fprintf(stdout,
		"========================= Start the Test Suite "
		"=========================\n\n");
	run_test(sr);
	pass = parse_results(sr);
	fprintf(stdout,
		"\n============================================================"
		"============\n\n");

	/* free ressources */
	srunner_free(sr);

	return pass == true ? 0 : 1;
}
