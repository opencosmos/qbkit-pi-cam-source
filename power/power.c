#include "power.h"

static const char fmt[] = "/qbkit/interfaces/payload_%s";

bool payload_power_set(const char *line, bool state)
{
	bool ret = false;
	char path[50];
	snprintf(path, sizeof(path), fmt, line);
	FILE *f = fopen(path, "w");
	if (!f) {
		logsysfail("Failed to open power line");
		goto fail;
	}
	if (fwrite(state ? "1" : "0", 1, 1, f) != 1) {
		logsysfail("Failed to set power state");
		goto fail1;
	}
	ret = true;
	goto done;
fail1:
	if (fclose(f)) {
		logsysfail("Failed to close power line");
	}
fail:
done:
	return ret;
}
