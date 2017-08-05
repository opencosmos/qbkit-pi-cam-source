#pragma once
#include <stdbool.h>
#include <stdio.h>



/****************************/
/*** Configuration begins ***/

/*
 * X-macro for generating test list (add tests to this if you want the runner to
 * know about them
 */
#define TEST_LIST_XMACRO \
	X(ping) \
	X(image) \

/* Define / uncomment to disable ANSI escape codes in output */
// #define TEST_NO_ANSI_TERM

/* Define / uncomment to use custom implementation of _test_log* functions */
// #define TEST_LOG_CUSTOM

/*
 * Define / uncomment if your "main" function should take no arguments (e.g.
 * embedded target).  Otherwise, the standard argc/argv will be used for
 * test_main.
 */
// #define TEST_MAIN_NOARGS

/* Define file where we log test info to (TEST_LOG_TARGET if NULL) */
extern FILE *test_log_target;

#if ! defined TEST_LOG_TARGET
#define TEST_LOG_TARGET stderr
#endif

/* Define to shorten logged source names to N path components */
#define TEST_LOG_SHORT_FILENAME 2
/* Define to trim extension off source name in log prefix */
#define TEST_LOG_STRIP_EXTENSION

/* Set to suppress logging of passed tests */
extern bool test_log_quiet;

/*** Configuration ends ***/
/**************************/



#define TEST_FUNC_SIGNATURE(name) void name(const void *arg)

/* Test suite function signature */
typedef TEST_FUNC_SIGNATURE(TestSuiteFunc);

/* Structure for holding test suite results */
struct TestCounter
{
	unsigned total;
	unsigned pass;
};

/* Used to define enumerable list of tests */
struct TestSuite
{
	/* Compile-time constant */
	const char *name;
	TestSuiteFunc * const func;
	/* Argument to test suite which may vary at run-time between runs */
	const void *arg;
	/* Set to true to skip this test */
	bool skip;
	/* For internal use */
	struct TestCounter counter;
};

/* Generate list of tests (last item is terminator with NULL for all fields) */
extern struct TestSuite *test_suites[];

/* Get suite by name (or NULL if not found) */
struct TestSuite *test_find_by_name(const char *name);

/* Get suite by index (or NULL if out of range) */
struct TestSuite *test_find_by_index(int index);

/* Run specific test suite */
void test_suite_run(struct TestSuite *suite);

/* Run all suites in order */
bool test_suites_run_all(bool stop_on_error);

/*
 * The checklist interface is intended to be used with shells, allowing a user
 * to select/deselect tests (by index), print the checklist to verify their
 * selection, then run the selected tests via test_suite_run_all, then (after
 * a long coffee break) view the results as a checklist in order to decide
 * whether to scroll back for error details or otherwise to enjoy a perfect test
 * run with another coffee.
 */
void test_checklist_print(const char *title);
bool test_checklist_clear(int index);
bool test_checklist_select(int index);
bool test_checklist_deselect(int index);
bool test_checklist_toggle(int index);
bool test_checklist_execute(int index);

/* Interactive shell that can be called from main program to expose tests */
void test_shell();

/* Macro to define a test */
#define TEST_DEFINE(suite_name) \
	struct TestSuite test_suite_##suite_name = { \
		.name = #suite_name, \
		.func = test_##suite_name \
	}; \
	TEST_FUNC_SIGNATURE(test_##suite_name)

/* Generate test function prototypes */
#define X(name) TEST_FUNC_SIGNATURE(test_##name);
TEST_LIST_XMACRO
#undef X

/* ANSI terminal codes */
#if defined TEST_NO_ANSI_TERM
#define TEST_ANSI_CODE(code) ""
#else
#define TEST_ANSI_CODE(code) "\x1b[" code "m"
#endif
#define TEST_ANSI_WRAP(pre, post, text) TEST_ANSI_CODE(pre) text TEST_ANSI_CODE(post)

/* Visual stuff */
#define TEST_ANSI_BOLD(text) TEST_ANSI_WRAP("1", "22", text)
#define TEST_ANSI_DARK(text) TEST_ANSI_WRAP("2", "22", text)
#define TEST_ANSI_ITALIC(text) TEST_ANSI_WRAP("4", "24", text)
#define TEST_ANSI_STRIKE(text) TEST_ANSI_WRAP("9", "29", text)
#define TEST_ANSI_FG_BLACK(text) TEST_ANSI_WRAP("29", "37", text)
#define TEST_ANSI_FG_RED(text) TEST_ANSI_WRAP("31", "37", text)
#define TEST_ANSI_FG_GREEN(text) TEST_ANSI_WRAP("32", "37", text)
#define TEST_ANSI_FG_YELLOW(text) TEST_ANSI_WRAP("33", "37", text)
#define TEST_ANSI_FG_BLUE(text) TEST_ANSI_WRAP("34", "37", text)
#define TEST_ANSI_FG_MAGENTA(text) TEST_ANSI_WRAP("35", "37", text)
#define TEST_ANSI_FG_CYAN(text) TEST_ANSI_WRAP("36", "37", text)
#define TEST_ANSI_FG_WHITE(text) TEST_ANSI_WRAP("37", "37", text)

/* Markers for test pass/fail */
#define TEST_PASS_SYMBOL TEST_ANSI_FG_GREEN("✓")
#define TEST_FAIL_SYMBOL TEST_ANSI_FG_RED("✗")
#define TEST_SKIP_SYMBOL " "

/* The following macros are lowercase, since they just proxy to functions */

/* Macro for assertion (calles test_{pass,fail} as needed) */
#define test_assert(name, expression) \
	test_assert_detail(name, "Assertion failed: " TEST_ANSI_ITALIC(#expression), expression)
#define test_assert_detail(name, description, expression) \
	_test_assert(__FILE__, __LINE__, __func__, name, description, expression)

/* Macros for logging test results during suite */
#define test_pass(name, description) \
	_test_pass(__FILE__, __LINE__, __func__, name, description)

#define test_fail(name, description) \
	_test_fail(__FILE__, __LINE__, __func__, name, description)

bool _test_assert(const char *file, const int line, const char *func, const char *name, const char *expression, bool value);
void _test_pass(const char *file, const int line, const char *func, const char *name, const char *description);
void _test_fail(const char *file, const int line, const char *func, const char *name, const char *description);

/* Logging proxies */
#define test_log(format, ...) \
	_test_log(__FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define test_log_nl() \
	_test_log_nl(__FILE__, __LINE__, __func__)

/* Unexpected error, e.g. invalid input, resource allocation failed */
#define test_error(format, ...) \
	_test_error(__FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

/* Basic logging functions */
void _test_log(const char *file, const int line, const char *func, const char *format, ...) __attribute__((format(printf, 4, 5)));
void _test_log_nl(const char *file, const int line, const char *func);
void _test_error(const char *file, const int line, const char *func, const char *format, ...) __attribute__((format(printf, 4, 5)));

/* Log-prefix format strings */
#ifndef TEST_LOG_FORMAT_STR
#define TEST_LOG_FORMAT_STR \
		" " \
		TEST_ANSI_FG_CYAN("[") \
		"%24.24s" \
		TEST_ANSI_FG_CYAN(":") \
		"%3u" \
		TEST_ANSI_FG_CYAN("]") \
		TEST_ANSI_FG_MAGENTA(" :: ") \
		"%-24.24s" \
		TEST_ANSI_FG_MAGENTA(" :: ") \
		""
#endif

#ifndef TEST_LOG_ERROR_FORMAT_STR
#define TEST_LOG_ERROR_FORMAT_STR \
		" " \
		TEST_ANSI_FG_CYAN("[") \
		TEST_ANSI_FG_RED("%24.24s") \
		TEST_ANSI_FG_CYAN(":") \
		TEST_ANSI_FG_RED("%3u") \
		TEST_ANSI_FG_CYAN("]") \
		TEST_ANSI_FG_MAGENTA(" :: ") \
		TEST_ANSI_FG_RED("%-24.24s") \
		TEST_ANSI_FG_MAGENTA(" :: ") \
		""
#endif
