#include "power.h"

static const char fmt[] = "/qbkit/interfaces/payload_%s";

bool payload_power_set(const char *line, bool state)
{
	bool ret = false;
	char path[50];
	snprintf(path, sizeof(path), fmt, line);
	int fd = open(path, O_WRONLY | O_SYNC);
	if (fd < 0) {
		logsysfail("Failed to open power line");
		goto fail;
	}
	if (write(fd, state ? "1" : "0", 1) != 1) {
		logsysfail("Failed to set power state");
		goto fail1;
	}
	ret = true;
fail1:
	if (close(fd)) {
		logsysfail("Failed to close power line");
	}
fail:
	return ret;
}
