#include <stdio.h>
#include <string.h>
#include "box.h"

unsigned box_width = 80;

#if defined BOX_NO_UNICODE

static void box_line()
{
	char line[box_width + 1];
	memset(line, '*', box_width);
	line[box_width] = 0;
	printf("%s\n", line);
}

void box_top()
{
	box_line();
	box_print("");
}

void box_bottom()
{
	box_print("");
	box_line();
}

void box_horiz()
{
	box_print("");
	box_line();
	box_print("");
}

void box_print(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	box_vprint(format, ap);
	va_end(ap);
}

void box_vprint(const char *format, va_list ap)
{
	char line[box_width + 1];
	line[0] = '*';
	unsigned len = vsnprintf(line + 1, box_width - 2, format, ap);
	if (len < box_width - 2) {
		memset(line + 1 + len, 32, box_width - 2 - len);
	}
	line[box_width - 1] = '*';
	line[box_width] = 0;
	printf("%s\n", line);
}

#else

#include "sprint.h"

#define BOX_NW "╭"
#define BOX_NE "╮"
#define BOX_SW "╰"
#define BOX_SE "╯"
#define BOX_V "│"
#define BOX_H "─"
#define BOX_W_T "├"
#define BOX_E_T "┤"


void box_top()
{
	if (box_width < 2) {
		return;
	}
	SPRINT_BUF(line, box_width * 4);
	sprint_buf_nstr(line, 1, BOX_NW);
	sprint_buf_nstr(line, box_width - 2, BOX_H);
	sprint_buf_nstr(line, 1, BOX_NE);
	printf("%s\n", line->buf);
}

void box_bottom()
{
	if (box_width < 2) {
		return;
	}
	SPRINT_BUF(line, box_width * 4);
	sprint_buf_nstr(line, 1, BOX_SW);
	sprint_buf_nstr(line, box_width - 2, BOX_H);
	sprint_buf_nstr(line, 1, BOX_SE);
	printf("%s\n", line->buf);
}

void box_horiz()
{
	if (box_width < 2) {
		return;
	}
	SPRINT_BUF(line, box_width * 4);
	sprint_buf_nstr(line, 1, BOX_W_T);
	sprint_buf_nstr(line, box_width - 2, BOX_H);
	sprint_buf_nstr(line, 1, BOX_E_T);
	printf("%s\n", line->buf);
}

void box_print(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	box_vprint(format, ap);
	va_end(ap);
}

void box_vprint(const char *format, va_list ap)
{
	if (box_width < 2) {
		return;
	}
	SPRINT_BUF(line, box_width * 4);
	sprint_buf_nstr(line, 1, BOX_V);
	vsprint_buf(line, format, ap);
	sprint_buf_u8_set_len(line, box_width - 1, ' ');
	sprint_buf_nstr(line, 1, BOX_V);
	printf("%s\n", line->buf);
}

#endif
