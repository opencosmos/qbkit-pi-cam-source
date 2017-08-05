#include "../stdinc.h"
#include "proto.h"

#define CS_BITS (8 * sizeof(CMD_READ_CS_TYPE))

CMD_READ_CS_TYPE calc_cs(const void *data, size_t length)
{
	CMD_READ_CS_TYPE cs = 0;
	for (const uint8_t *it = data, *end = it + length; it != end; ++it) {
		cs = cs << 3 | cs >> (CS_BITS - 3);
		cs ^= *it;
	}
	return ~cs;
}
