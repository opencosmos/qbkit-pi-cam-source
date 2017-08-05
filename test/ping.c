#include <test.h>
#include "../stdinc.h"
#include "../payload/client.h"
#include "../payload/server.h"
#include "../uart/proto.h"
#include "../uart/pipe.h"

static void *server_thread(void *arg)
{
	struct uart_context *uart = arg;
	const void *packet;
	while (pthread_testcancel(), (packet = uart_rx_packet(uart))) {
		svr_dispatch(uart, packet);
	}
	return NULL;
}

TEST_DEFINE(ping)
{
	struct uart_context uart_client;
	struct uart_context uart_server;
	pthread_t server_tid;
	pthread_create(&server_tid, NULL, server_thread, &uart_server);
	void *pipe = create_sync_uart_pipe(&uart_server, &uart_client, UART_FRAME_SIZE + 50);

	test_assert("Ping/pong via mock UART", cmd_ping(&uart_client));
	test_assert("Ping/pong via mock UART", cmd_ping(&uart_client));
	test_assert("Ping/pong via mock UART", cmd_ping(&uart_client));

	pthread_cancel(server_tid);
	pthread_join(server_tid, NULL);

	free(pipe);
}
