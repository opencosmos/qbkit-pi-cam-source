#include "pipe.h"
#include <pthread.h>

static void async_pipe_on_tx_byte(struct uart_context *ctx, uint8_t byte)
{
	struct uart_context *other = ctx->extra;
	uart_on_rx_byte(other, byte);
}

void create_async_uart_pipe(struct uart_context *a, struct uart_context *b)
{
	memset(a, 0, sizeof(*a));
	memset(b, 0, sizeof(*b));
	a->on_tx_byte = async_pipe_on_tx_byte;
	b->on_tx_byte = async_pipe_on_tx_byte;
	a->extra = b;
	b->extra = a;
}

struct sync_pipe_end
{
	pthread_mutex_t mx;
	pthread_cond_t cv;
	struct uart_context *uart;
	size_t idx;
	size_t size;
	uint8_t *buf;
};

struct sync_pipe
{
	struct sync_pipe_end end[2];
	uint8_t buf[];
};

static void sync_pipe_on_tx_byte(struct uart_context *ctx, uint8_t byte)
{
	struct sync_pipe *state = ctx->extra;
	struct sync_pipe_end *tx_end = &state->end[state->end[0].uart == ctx ? 0 : 1];
	pthread_mutex_lock(&tx_end->mx);
	if (tx_end->idx == tx_end->size) {
		logfail("Buffer overrun, data lost (buffer size is %zu)", tx_end->size);
	}
	tx_end->buf[tx_end->idx++] = byte;
	pthread_cond_signal(&tx_end->cv);
	pthread_mutex_unlock(&tx_end->mx);
}

static bool sync_pipe_on_rx_pull(struct uart_context *ctx)
{
	struct sync_pipe *state = ctx->extra;
	struct sync_pipe_end *rx_end = &state->end[state->end[0].uart == ctx ? 1 : 0];
	bool done = false;
	pthread_mutex_lock(&rx_end->mx);
	size_t idx = 0;
	do {
		while (rx_end->idx == 0) {
			pthread_cond_wait(&rx_end->cv, &rx_end->mx);
		}
		for (idx = 0; !done && idx < rx_end->idx; ++idx) {
			done = uart_on_rx_byte(ctx, rx_end->buf[idx]);
		}
	} while (!done);
	memmove(&rx_end->buf[0], &rx_end->buf[idx], rx_end->idx - idx);
	rx_end->idx = 0;
	pthread_mutex_unlock(&rx_end->mx);
	return true;
}

void *create_sync_uart_pipe(struct uart_context *a, struct uart_context *b, size_t bufsize)
{
	struct sync_pipe *state = malloc(sizeof(*state) + bufsize * 2);
	pthread_mutex_init(&state->end[0].mx, NULL);
	pthread_cond_init(&state->end[0].cv, NULL);
	state->end[0].uart = a;
	state->end[0].idx = 0;
	state->end[0].size = bufsize;
	state->end[0].buf = state->buf;
	pthread_mutex_init(&state->end[1].mx, NULL);
	pthread_cond_init(&state->end[1].cv, NULL);
	state->end[1].uart = b;
	state->end[1].idx = 0;
	state->end[1].size = bufsize;
	state->end[1].buf = state->buf + bufsize;
	memset(a, 0, sizeof(*a));
	memset(b, 0, sizeof(*b));
	a->on_tx_byte = sync_pipe_on_tx_byte;
	b->on_tx_byte = sync_pipe_on_tx_byte;
	a->on_rx_pull = sync_pipe_on_rx_pull;
	b->on_rx_pull = sync_pipe_on_rx_pull;
	a->extra = state;
	b->extra = state;
	return state;
}

void destroy_sync_uart_pipe(void *pipe)
{
	struct sync_pipe *state = pipe;
	pthread_mutex_destroy(&state->end[0].mx);
	pthread_cond_destroy(&state->end[0].cv);
	pthread_mutex_destroy(&state->end[1].mx);
	pthread_cond_destroy(&state->end[1].cv);
}
