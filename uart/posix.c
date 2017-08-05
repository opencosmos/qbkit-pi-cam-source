#include "posix.h"

int read_timeout_ms = 3000;

static void file_on_tx_byte(struct uart_context *ctx, uint8_t byte)
{
	int fd = (intptr_t) ctx->extra;
	if (write(fd, &byte, 1) != 1) {
		logsysfail("write");
		exit(1);
	}
}

static bool wait_for_byte(int fd, int timeout_ms)
{
	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLIN;
	return poll(&pfd, 1, timeout_ms) == 1 && (pfd.revents & POLLIN);
}

static bool file_on_rx_pull(struct uart_context *ctx)
{
	int fd = (intptr_t) ctx->extra;
	bool ready;
	int ret = 0;
	uint8_t c;
	while ((ready = wait_for_byte(fd, read_timeout_ms)) &&
			(ret = read(fd, &c, 1)) == 1) {
		if (uart_on_rx_byte(ctx, c)) {
			return true;
		}
	}
	if (ready && ret != 0) {
		logsysfail("read");
		exit(1);
	}
	return false;
}

void map_uart_file(struct uart_context *uart, int fd)
{
	uart->extra = (void *) (intptr_t) fd;
	uart->on_tx_byte = file_on_tx_byte;
	uart->on_rx_pull = file_on_rx_pull;
}
