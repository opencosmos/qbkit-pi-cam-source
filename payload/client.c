#include "../stdinc.h"
#include "../endian.h"
#include "client.h"

#define MAX_READ_RETRIES 10

bool cmd_ping(struct uart_context *uart)
{
	struct wire_cmd_req_header _req = {
		.cmd = CMD_PING
	};
	uart_tx_data(uart, &_req, sizeof(_req));
	const struct uart_packet *p = uart_rx_packet(uart);
	const struct wire_cmd_res_header *_res = (const void *) p->data;
	if (!p || _res->cmd != _req.cmd || !_res->success) {
		return false;
	}
	return true;
}

bool cmd_poweroff(struct uart_context *uart)
{
	struct wire_cmd_req_header _req = {
		.cmd = CMD_POWEROFF
	};
	uart_tx_data(uart, &_req, sizeof(_req));
	const struct uart_packet *p = uart_rx_packet(uart);
	const struct wire_cmd_res_header *_res = (const void *) p->data;
	if (!p || _res->cmd != _req.cmd || !_res->success) {
		return false;
	}
	return true;
}

bool cmd_capture(struct uart_context *uart, struct cmd_capture_req *req, struct cmd_capture_res *res)
{
	(void) req;
	(void) res;
	struct wire_cmd_capture_req _req = {
		.hdr = { .cmd = CMD_CAPTURE }
	};
	strncpy(_req.name, req->name, CAPTURE_FILENAME_LEN);
	uart_tx_data(uart, &_req, sizeof(_req));
	const struct uart_packet *p = uart_rx_packet(uart);
	const struct wire_cmd_capture_res *_res = (const void *) p->data;
	if (!p || _res->hdr.cmd != _req.hdr.cmd || !_res->hdr.success) {
		return false;
	}
	return true;
}

bool cmd_get_length(struct uart_context *uart, struct cmd_get_length_req *req, struct cmd_get_length_res *res)
{
	(void) req;
	struct wire_cmd_get_length_req _req = {
		.hdr = { .cmd = CMD_GET_LENGTH }
	};
	strncpy(_req.name, req->name, CAPTURE_FILENAME_LEN);
	uart_tx_data(uart, &_req, sizeof(_req));
	const struct uart_packet *p = uart_rx_packet(uart);
	const struct wire_cmd_get_length_res *_res = (const void *) p->data;
	if (!p || _res->hdr.cmd != _req.hdr.cmd || !_res->hdr.success) {
		return false;
	}
	res->length = swap32(_res->length);
	return true;
}

bool cmd_read(struct uart_context *uart, struct cmd_read_req *req, struct cmd_read_res *res)
{
	struct wire_cmd_read_req _req = {
		.hdr = { .cmd = CMD_READ },
		.offset = swap32(req->offset)
	};
	strncpy(_req.name, req->name, CAPTURE_FILENAME_LEN);
	uart_tx_data(uart, &_req, sizeof(_req));
	const struct uart_packet *p = uart_rx_packet(uart);
	const struct wire_cmd_read_res *_res = (const void *) p->data;
	if (!p || _res->hdr.cmd != _req.hdr.cmd || !_res->hdr.success) {
		return false;
	}
	const size_t blklen = p->size - CMD_READ_FRAMING_SIZE;
	memcpy(res->data, _res->data, blklen);
#if defined CS_DEBUG
	loginfo("svr_read: %zu bytes, crc=%04hx", blklen, _res->cs);
#endif
	res->cs = CMD_READ_CS_SWAP(_res->cs);
	res->size = blklen;
	return true;
}

bool cmd_capture_and_save_image(struct uart_context *ctx, const char *name, const char *destname)
{
	struct cmd_capture_req cq;
	struct cmd_capture_res cr;
	snprintf(cq.name, sizeof(cq.name), "%s", name);
	if (!cmd_capture(ctx, &cq, &cr)) {
		logfail("Failed to capture image");
		return false;
	}
	struct cmd_get_length_req glq;
	struct cmd_get_length_res glr;
	snprintf(glq.name, sizeof(glq.name), "%s", name);
	if (!cmd_get_length(ctx, &glq, &glr)) {
		logfail("Failed to get image length");
		return false;
	}
	FILE *f = fopen(destname, "w");
	if (!f) {
		logsysfail("fopen");
		return false;
	}
	const size_t length = glr.length;
	size_t count = 0;
	int last_percent = -1;
	struct cmd_read_req crq;
	snprintf(crq.name, sizeof(crq.name), "%s", name);
	const time_t t0 = time(NULL);
	unsigned retry = 0;
	while (count < length) {
		crq.offset = count;
		struct cmd_read_res crr;
		if (!cmd_read(ctx, &crq, &crr)) {
			logfail("Failed to read block");
			fclose(f);
			return false;
		}
		const size_t remaining = length - count;
		const size_t blklen = remaining > CMD_READ_BLOCK_SIZE ? CMD_READ_BLOCK_SIZE : remaining;
		if (calc_cs(crr.data, blklen) != crr.cs) {
			if (retry == MAX_READ_RETRIES) {
				logfail("Retry limit reached");
				fclose(f);
				return false;
			}
			retry++;
			logfail("Checksum verification failed, retrying (retry #%u)", retry);
			continue;
		}
		retry = 0;
		if (fwrite(crr.data, blklen, 1, f) != 1) {
			logsysfail("fwrite");
			fclose(f);
			return false;
		}
		count += blklen;
		int percent = (count * 100) / length;
		if (percent > last_percent && percent % 5 == 0) {
			last_percent = percent;
			loginfo("Download progress: %d%% (%.1fk of %.1fk)\n", percent, count / 1024.0, length / 1024.0);
		}
	}
	const time_t dt = time(NULL) - t0;
	if (dt > 0) {
		loginfo("Data rate: %.1fkb/s over %us", length / 1024.0 / dt, (unsigned) dt);
	}
	fclose(f);
	return true;
}
