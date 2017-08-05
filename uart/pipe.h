#pragma once
#include "../stdinc.h"
#include "proto.h"

/* Pipe implementation behind uart_proto, for testing */

/*
 * Writes go from one pipe to the other immediately, so only the asynchronous
 * access pattern will work
 */
void create_async_uart_pipe(struct uart_context *a, struct uart_context *b);

/*
 * Destroy when done.
 *
 * Writes go to temporary buffer and are flushed on demand via on_rx_pull,
 * allowing synchronous reception via uart_rx_packet.
 *
 * Can only buffer one packet at a time in each direction.
 */
void *create_sync_uart_pipe(struct uart_context *a, struct uart_context *b, size_t bufsize);

void destroy_sync_uart_pipe(void *pipe);
