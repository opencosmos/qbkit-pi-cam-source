#include "../stdinc.h"
#include "../endian.h"
#include "../uart/proto.h"
#include "proto.h"
#include "vars.h"
#include "conf.h"

const char *capture_dir = "/var/cache/picam";
const char *capture_cmd_fmt = NULL;

static const char *get_capture_file(const char name_ser[CAPTURE_FILENAME_LEN])
{
	static char buf[60];
	char name[CAPTURE_FILENAME_LEN + 1];
	name[CAPTURE_FILENAME_LEN] = 0;
	strncpy(name, name_ser, CAPTURE_FILENAME_LEN);
	snprintf(buf, sizeof(buf), "%s/%s", capture_dir, name);
	return buf;
}

static size_t svr_ping(const struct wire_cmd_req_header *req, struct wire_cmd_res_header *res)
{
	(void) req;
	(void) res;
	loginfo("ping");
	return sizeof(*res);
}

static size_t svr_poweroff(const struct wire_cmd_req_header *req, struct wire_cmd_res_header *res)
{
	(void) req;
	(void) res;
	loginfo("poweroff");
	if (system("poweroff") != 0) {
		logsysfail("poweroff");
		return 0;
	}
	return sizeof(*res);
}

static size_t svr_capture(const struct wire_cmd_capture_req *req, struct wire_cmd_capture_res *res)
{
	(void) req;
	(void) res;
	loginfo("capture");
	char command[200];
	if (!capture_cmd_fmt) {
		logfail("No capture command has been set");
		return 0;
	}
	snprintf(command, sizeof(command), capture_cmd_fmt, get_capture_file(req->name));
	loginfo("exec: %s", command);
	return system(command) == 0 ? sizeof(*res) : 0;
}

static size_t svr_get_length(const struct wire_cmd_get_length_req *req, struct wire_cmd_get_length_res *res)
{
	(void) req;
	loginfo("get_length");
	FILE *f = fopen(get_capture_file(req->name), "r");
	if (!f) {
		return 0;
	}
	if (fseek(f, 0, SEEK_END) != 0) {
		logsysfail("fseek");
		fclose(f);
		return 0;
	}
	const long length = ftell(f);
	if (length == -1) {
		logsysfail("ftell");
		fclose(f);
		return 0;
	}
	fclose(f);
	res->length = swap32(length);
	return sizeof(*res);
}

static size_t svr_read(const struct wire_cmd_read_req *req, struct wire_cmd_read_res *res)
{
	loginfo("read");
	const size_t offset = swap32(req->offset);
	FILE *f = fopen(get_capture_file(req->name), "r");
	if (fseek(f, offset, SEEK_SET) != 0) {
		logsysfail("fseek");
		fclose(f);
		return 0;
	}
	const size_t length = fread(res->data, 1, CMD_READ_BLOCK_SIZE, f);
	const bool fail = (length != CMD_READ_BLOCK_SIZE && !feof(f)) || ferror(f);
	fclose(f);
	if (fail) {
		logsysfail("fread");
		return 0;
	}
	res->cs = CMD_READ_CS_SWAP(calc_cs(res->data, length));
#if defined CS_DEBUG
	loginfo("svr_read: %zu bytes, crc=%04hx", length, res->cs);
#endif
	return CMD_READ_FRAMING_SIZE + length;
}

bool svr_dispatch(struct uart_context *uart, const struct uart_packet *req)
{
	const struct wire_cmd_req_header *_req = (const void *) req->data;
	union __attribute__((__packed__)) {
		char data[UART_PACKET_SIZE];
		struct wire_cmd_res_header hdr;
	} _res;
	memset(&_res, 0, sizeof(_res));
	size_t reslen = 0;
	loginfo("Processing command 0x%02x", _req->cmd);
	switch (_req->cmd) {
	case CMD_PING:
		reslen = svr_ping((void *) _req, (void *) &_res);
		break;
	case CMD_POWEROFF:
		reslen = svr_poweroff((void *) _req, (void *) &_res);
		break;
	case CMD_CAPTURE:
		reslen = svr_capture((void *) _req, (void *) &_res);
		break;
	case CMD_GET_LENGTH:
		reslen = svr_get_length((void *) _req, (void *) &_res);
		break;
	case CMD_READ:
		reslen = svr_read((void *) _req, (void *) &_res);
		break;
	default:
		logfail("Invalid command: 0x%02hhx\n", _req->cmd);
		return false;
	}
	const bool success = reslen > 0;
	_res.hdr.cmd = _req->cmd;
	_res.hdr.success = success;
	if (reslen == 0) {
		logfail("Failed to execute command 0x%02hhx\n", _req->cmd);
	}
	loginfo("Sending response");
	uart_tx_data(uart, &_res, reslen ? reslen : sizeof(_res.hdr));
	loginfo("Complete");
	return success;
}
