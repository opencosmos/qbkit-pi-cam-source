#pragma once
#include "stdinc.h"

static inline bool is_big_endian()
{
	union {
		uint16_t w;
		uint8_t m;
	} test = { .w = 0x0100 };
	return test.m;
}

static inline uint16_t swap16(uint16_t x)
{
	if (is_big_endian()) {
		return x;
	}
	return x >> 8 | x << 8;
}

static inline uint32_t swap32(uint32_t x)
{
	if (is_big_endian()) {
		return x;
	}
	return swap16(x >> 16) | swap16(x) << 16;
}
