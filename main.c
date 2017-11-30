#include "uart/posix.h"
#include "payload/client.h"
#include "payload/server.h"
#include "payload/power.h"
#include "payload/vars.h"
#include "payload/conf.h"

#define BAUD B230400

static void init_serial(int fd, int baud)
{
        struct termios tty;
        memset(&tty, 0, sizeof(tty));

        if (tcgetattr(fd, &tty) != 0) {
		logsysfail("tcgetattr");
		return;
        }

	/* Set raw (8-bit, no translation, no parity) */
	cfmakeraw(&tty);

        if (cfsetospeed(&tty, baud) == -1) {
		logsysfail("cfsetospeed");
	}
	if (cfsetispeed(&tty, baud) == -1) {
		logsysfail("cfsetispeed");
	}

	/* Read must return at least 1-byte (unless timeout/error/eof occurs) */
        tty.c_cc[VMIN]  = 1;
	/* No interbyte read timeout */
        tty.c_cc[VTIME] = 0;

	/* No xon/xoff */
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);

	/* No modem controls, enable reading */
        tty.c_cflag |= (CLOCAL | CREAD);

	/* No stop bits / RTS / CTS */
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr(fd, TCSANOW, &tty) != 0) {
		logsysfail("tcsetattr");
		return;
        }
}

static bool run_server(struct uart_context *uart)
{
	const char *driver;
	if (getenv("USE_V4L")) {
		driver = "v4l";
		capture_cmd_fmt = CAPTURE_V4L;
	} else {
		driver = "picam";
		capture_cmd_fmt = CAPTURE_PICAM;
	}
	loginfo("Using %s driver, exec = %s", driver, capture_cmd_fmt);
	const struct uart_packet *packet;
	/* Ignore timeouts */
	while (true) {
		while ((packet = uart_rx_packet(uart))) {
			loginfo("Packet received");
			svr_dispatch(uart, packet);
		}
	}
	loginfo("Server stopping");
	return true;
}

static bool run_client(struct uart_context *uart, const char *name, const char *path, int width, int height)
{
	char dest[100];
	snprintf(dest, sizeof(dest), "%s/%s", path, name);
	if (!cmd_capture_and_save_image(uart, name, dest, width, height)) {
		logfail("Failed");
		return false;
	}
	loginfo("Image %s captured and saved to file %s", name, dest);
	return true;
}

int main(int argc, char *argv[])
{
	static struct uart_context uart;
	if (argc < 3) {
		goto syntax;
	}
	bool client = strcmp(argv[1], "client") == 0;
	bool server = strcmp(argv[1], "server") == 0;
	bool ping = strcmp(argv[1], "ping") == 0;
	if (client + server + ping != 1) {
		goto syntax;
	}
	if (server && argc != 3) {
		goto syntax;
	}
	if (client && argc != 5 && argc != 7) {
		goto syntax;
	}
	if (ping && argc != 3) {
		goto syntax;
	}
	int fd = open(argv[2], O_RDWR | O_SYNC | O_NOCTTY);
	if (fd == -1) {
		logsysfail("open");
		return 1;
	}
	init_serial(fd, BAUD);
	map_uart_file(&uart, fd);
	bool res;
	if (ping) {
		struct timespec t0;
		struct timespec t1;
		clock_gettime(CLOCK_MONOTONIC, &t0);
		res = cmd_ping(&uart);
		clock_gettime(CLOCK_MONOTONIC, &t1);
		uint64_t ns = (t1.tv_sec - t0.tv_sec) * 1000000000 + ((int64_t) t1.tv_nsec - t0.tv_nsec);
		if (res) {
			loginfo("Ping! %.3fms", ns * 1e-6);
		} else {
			logfail("No reply to ping");
		}
	} else if (client) {
		const char *name = argv[3];
		const char *path = argv[4];
		const int width = argc == 7 ? atoi(argv[5]) : 640;
		const int height = argc == 7 ? atoi(argv[6]) : 480;
		const bool mock_power = getenv("MOCK_POWER");
		if (!mock_power && !payload_power_on(&uart)) {
			logfail("Payload failed to start up");
		}
		res = run_client(&uart, name, path, width, height);
		if (!mock_power && !payload_power_off(&uart)) {
			logfail("Payload failed to power down");
		}
	} else {
		res = run_server(&uart);
	}
	close(fd);
	if (!res) {
		logfail("Program failed");
	}
	return res ? 0 : 3;
syntax:
	logfail("Syntax: %s ping <uart>", argv[0]);
	logfail("Syntax: %s server <uart>", argv[0]);
	logfail("Syntax: %s client <uart> <image-name> <save-path> [<width> <height>]", argv[0]);
	return 2;
}
