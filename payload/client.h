#pragma once
#include "../stdinc.h"
#include "../uart/proto.h"
#include "proto.h"

bool cmd_ping(struct uart_context *uart);
bool cmd_poweroff(struct uart_context *uart);
bool cmd_capture(struct uart_context *uart, struct cmd_capture_req *req, struct cmd_capture_res *res);
bool cmd_get_length(struct uart_context *uart, struct cmd_get_length_req *req, struct cmd_get_length_res *res);
bool cmd_read(struct uart_context *uart, struct cmd_read_req *req, struct cmd_read_res *res);
bool cmd_capture_and_save_image(struct uart_context *ctx, const char *name, const char *destname);
