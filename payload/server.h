#pragma once
#include "../stdinc.h"
#include "../uart/proto.h"
#include "proto.h"

bool svr_dispatch(struct uart_context *uart, const struct uart_packet *req);
