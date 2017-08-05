#include "sprint.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <memory.h>

void sprint_buf_init(struct sprint_buf *spb, size_t capacity)
{
	spb->capacity = capacity;
	spb->pos = 0;
	spb->buf[0] = 0;
}

void sprint_buf(struct sprint_buf *spb, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vsprint_buf(spb, format, ap);
	va_end(ap);
}

void vsprint_buf(struct sprint_buf *spb, const char *format, va_list args)
{
	const size_t remain = 1 + spb->capacity - spb->pos;
	size_t len = vsnprintf(spb->buf + spb->pos, remain, format, args);
	if (len >= remain) {
		len = remain - 1;
	}
	spb->pos += len;
}

void sprint_buf_nchar(struct sprint_buf *spb, size_t n, char c)
{
	for ( ; n > 0 && spb->pos < spb->capacity; --n) {
		spb->buf[spb->pos] = c;
		spb->pos++;
	}
	spb->buf[spb->pos] = 0;
}

void sprint_buf_nstr(struct sprint_buf *spb, size_t n, const char *s)
{
	const size_t len = strlen(s);
	size_t rem = spb->capacity - spb->pos;
	for ( ; n > 0 && rem > 0; --n) {
		if (rem >= len) {
			memcpy(spb->buf + spb->pos, s, len);
			spb->pos += len;
			rem -= len;
		} else {
			memcpy(spb->buf + spb->pos, s, rem);
			spb->pos += rem;
			rem = 0;
		}
	}
	spb->buf[spb->pos] = 0;
}

static bool is_u8_cont(char c)
{
	return (c & 0xc0) == 0x80;
}

size_t sprint_buf_u8_len(struct sprint_buf *spb)
{
	size_t len = 0;
	for (size_t i = 0; i < spb->pos; ++i) {
		if (!is_u8_cont(spb->buf[i])) {
			len++;
		}
	}
	return len;
}

void sprint_buf_u8_set_len(struct sprint_buf *spb, size_t len, char fill)
{
	size_t count = 0;
	size_t i;
	for (i = 0; i < spb->pos; ++i) {
		if (!is_u8_cont(spb->buf[i])) {
			if (count == len) {
				spb->buf[i] = 0;
				spb->pos = i;
				return;
			}
			count++;
		}
	}
	size_t grow = len - count;
	if (spb->pos + grow > spb->capacity) {
		grow = spb->capacity - spb->pos;
	}
	memset(spb->buf + spb->pos, fill, grow);
	spb->pos += grow;
	spb->buf[spb->pos] = 0;
}

char *sprint_buf_u8_pos(struct sprint_buf *spb, size_t pos)
{
	size_t count = 0;
	size_t i;
	for (i = 0; i < spb->pos && count < pos; ++i) {
		if (!is_u8_cont(spb->buf[i])) {
			count++;
		}
	}
	return &spb->buf[i];
}
