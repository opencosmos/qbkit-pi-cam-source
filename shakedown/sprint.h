#pragma once
#include <stddef.h>
#include <stdarg.h>

struct sprint_buf
{
	size_t capacity;
	size_t pos;
	char buf[];
};

#define SPRINT_BUF_SIZE(capacity) (sizeof(struct sprint_buf) + capacity + 1)
#define SPRINT_BUF(name, capacity) \
	size_t _##name##_len = (capacity); \
	char _##name##_buf[SPRINT_BUF_SIZE(_##name##_len)]; \
	struct sprint_buf *name = (void *) &_##name##_buf; \
	sprint_buf_init(name, _##name##_len);

void sprint_buf_init(struct sprint_buf *spb, size_t capacity);
void sprint_buf(struct sprint_buf *spb, const char *format, ...) __attribute__((format(printf, 2, 3)));
void vsprint_buf(struct sprint_buf *spb, const char *format, va_list args);

void sprint_buf_nchar(struct sprint_buf *spb, size_t n, char c);
void sprint_buf_nstr(struct sprint_buf *spb, size_t n, const char *s);

size_t sprint_buf_u8_len(struct sprint_buf *spb);
void sprint_buf_u8_set_len(struct sprint_buf *spb, size_t len, char fill);
char *sprint_buf_u8_pos(struct sprint_buf *spb, size_t pos);
