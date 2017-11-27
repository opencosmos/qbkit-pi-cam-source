#include <test.h>
#include "../stdinc.h"
#include "../payload/client.h"
#include "../payload/server.h"
#include "../payload/vars.h"
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

TEST_DEFINE(image)
{
	struct uart_context uart_client;
	struct uart_context uart_server;
	pthread_t server_tid;
	pthread_create(&server_tid, NULL, server_thread, &uart_server);
	void *pipe = create_sync_uart_pipe(&uart_server, &uart_client, UART_FRAME_SIZE + 50);

	capture_cmd_fmt = "echo size=%dx%d && touch %s";
	capture_dir = "/home/mark/Pictures";

	{
		test_assert("Ping/pong via mock UART", cmd_ping(&uart_client));
	}

	{
		struct cmd_capture_req cq;
		struct cmd_capture_res cr;
		cq.width = 100;
		cq.height = 75;
		snprintf(cq.name, sizeof(cq.name), "%s", "deadpool-unicorn.jpg");
		test_assert("Capture image (mocked implementation)", cmd_capture(&uart_client, &cq, &cr));
	}

	{
		struct cmd_get_length_req glq;
		struct cmd_get_length_res glr;
		snprintf(glq.name, sizeof(glq.name), "%s", "deadpool-unicorn.jpg");
		test_assert("Get image size", cmd_get_length(&uart_client, &glq, &glr));
		test_assert("Correct image size", glr.length == 48160);
	}

	{
		struct cmd_read_req rq;
		struct cmd_read_res rr;
		rq.offset = 0;
		snprintf(rq.name, sizeof(rq.name), "%s", "deadpool-unicorn.jpg");
		test_assert("Read block from image", cmd_read(&uart_client, &rq, &rr));
		test_assert("Verify checksum", calc_cs(rr.data, rr.size) == rr.cs);
	}

	pthread_cancel(server_tid);
	pthread_join(server_tid, NULL);
	free(pipe);
}
