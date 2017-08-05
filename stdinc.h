#pragma once
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <termios.h>
#include <poll.h>

#define logsysfail(call) \
	logfail("Call %s failed: %s (%d)", call, strerror(errno), errno)

#define logfail(format, ...) \
	fprintf(stderr, "[fail] %s:%d(%s) : " format "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#define loginfo(format, ...) \
	fprintf(stderr, "[info] " format "\n", ##__VA_ARGS__)
