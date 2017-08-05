#include "test.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define SYNTAX_CHECK(nfields) \
	do { \
		if (fields != nfields) { \
			printf("Syntax error, incorrect parameter count\n"); \
			continue; \
		} \
	} while (0)

#define RANGE_CHECK(val) \
	do { \
		if (!(val)) { \
			printf("Range error, invalid test index\n"); \
			continue; \
		} \
	} while (0)

static int get_command(char *cmd, int *arg)
{
	char buf[40];
	if (fgets(buf, sizeof(buf), stdin) == NULL) {
		return EOF;
	}
	int len = strlen(buf);
	if (len == 0 || buf[0] == '\n') {
		return 0;
	}
	if (buf[len - 1] == '\n') {
		buf[--len] = 0;
	}
	*cmd = buf[0];
	if (strlen(buf) == 1) {
		return 1;
	}
	if (strlen(buf) == 2) {
		return 0;
	}
	*arg = atoi(buf + 2);
	return 2;
}

void test_shell()
{
	bool bail = false;
	bool quit = false;
	while (!quit) {
		char c;
		int idx = -2;
		printf("test$ ");
		int fields = get_command(&c, &idx);
		if (fields == EOF) {
			break;
		}
		if (fields == 0) {
			continue;
		}
		printf("\n");
		switch (c) {
		case 'h':
			SYNTAX_CHECK(1);
			printf(TEST_ANSI_BOLD("Test shell help:\n"));
			printf(" " TEST_ANSI_FG_CYAN("%c") "   - %s\n", 'h', "Show this help");
			printf(" " TEST_ANSI_FG_CYAN("%c") "   - %s\n", 'q', "Quit test shell");
			printf(" " TEST_ANSI_FG_CYAN("%c") "   - %s\n", 'l', "Show test checklist");
			printf(" " TEST_ANSI_FG_CYAN("%c") "   - %s\n", 'r', "Run test suites");
			printf(" " TEST_ANSI_FG_CYAN("%c") "   - %s\n", 'b', "Toggle bail-on-first-failed-suite");
			printf(" " TEST_ANSI_FG_CYAN("%c") " " TEST_ANSI_FG_MAGENTA("n") " - %s\n", 'c', "Clear test suite result");
			printf(" " TEST_ANSI_FG_CYAN("%c") " " TEST_ANSI_FG_MAGENTA("n") " - %s\n", 'e', "Execute test suite");
			printf(" " TEST_ANSI_FG_CYAN("%c") " " TEST_ANSI_FG_MAGENTA("n") " - %s\n", 't', "Toggle test suite");
			printf(" " TEST_ANSI_FG_CYAN("%c") " " TEST_ANSI_FG_MAGENTA("n") " - %s\n", 's', "Select test suite");
			printf(" " TEST_ANSI_FG_CYAN("%c") " " TEST_ANSI_FG_MAGENTA("n") " - %s\n", 'd', "Deselect test suite");
			printf("\n");
			printf(" Specify value of " TEST_ANSI_FG_CYAN("-1") " for " TEST_ANSI_FG_MAGENTA("n") " to apply the command to all tests (in order)\n");
			printf("\n");
			break;
		case 'q':
			SYNTAX_CHECK(1);
			quit = true;
			break;
		case 'l':
			SYNTAX_CHECK(1);
			test_checklist_print("Test checklist");
			break;
		case 'r':
			SYNTAX_CHECK(1);
			test_suites_run_all(bail);
			break;
		case 'b':
			SYNTAX_CHECK(1);
			bail = !bail;
			printf("Test runner will %sbail on first failed suite\n", bail ? "" : "not ");
			printf("\n");
			break;
		case 'c':
			SYNTAX_CHECK(2);
			RANGE_CHECK(test_checklist_clear(idx));
			break;
		case 'e':
			SYNTAX_CHECK(2);
			RANGE_CHECK(test_checklist_execute(idx));
			break;
		case 't':
			SYNTAX_CHECK(2);
			RANGE_CHECK(test_checklist_toggle(idx));
			break;
		case 's':
			SYNTAX_CHECK(2);
			RANGE_CHECK(test_checklist_select(idx));
			break;
		case 'd':
			SYNTAX_CHECK(2);
			RANGE_CHECK(test_checklist_deselect(idx));
			break;
		default:
			printf(TEST_ANSI_FG_RED("Invalid command: '%c'\n"), c);
			printf("\n");
			break;
		}
	}
}
