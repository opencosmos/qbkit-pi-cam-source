#pragma once
#include <stdarg.h>

void box_top();
void box_bottom();
void box_horiz();
void box_print(const char *format, ...);
void box_vprint(const char *format, va_list ap);

/* No unicode support */
// #define BOX_NO_UNICODE
