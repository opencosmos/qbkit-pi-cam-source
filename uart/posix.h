#pragma once
#include "../stdinc.h"
#include "proto.h"

extern int read_timeout_ms;

void map_uart_file(struct uart_context *uart, int fd);
