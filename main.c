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

static bool run_client(struct uart_context *uart, const char *name, const char *path)
{
	char dest[100];
	snprintf(dest, sizeof(dest), "%s/%s", path, name);
	if (!cmd_capture_and_save_image(uart, name, dest)) {
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
	if (client == server) {
		goto syntax;
	}
	if (server && argc != 3) {
		goto syntax;
	}
	if (client && argc != 5) {
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
	if (client) {
		const char *name = argv[3];
		const char *path = argv[4];
		if (!payload_power_on(&uart)) {
			logfail("Payload failed to start up");
		}
		res = run_client(&uart, name, path);
		if (!payload_power_off(&uart)) {
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
	logfail("Syntax: %s server <uart>", argv[0]);
	logfail("Syntax: %s client <uart> <image-name> <save-path>", argv[0]);
	return 2;
}
