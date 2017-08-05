#pragma once
#include "../stdinc.h"
#include "../uart/proto.h"

#define CMD_PING 0x00
#define CMD_POWEROFF 0xca
#define CMD_CAPTURE 0x10
#define CMD_GET_LENGTH 0x20
#define CMD_READ 0x21

struct __attribute__((__packed__)) wire_cmd_req_header
{
	uint8_t cmd;
};

struct __attribute__((__packed__)) wire_cmd_res_header
{
	uint8_t cmd;
	uint8_t success;
};


#define CAPTURE_FILENAME_LEN 40
#define CMD_READ_CS_TYPE uint16_t
#define CMD_READ_CS_SWAP swap16
#define CMD_READ_FRAMING_SIZE (sizeof(struct wire_cmd_res_header) + sizeof(CMD_READ_CS_TYPE))
#define CMD_READ_BLOCK_SIZE (UART_PACKET_SIZE - CMD_READ_FRAMING_SIZE)

CMD_READ_CS_TYPE calc_cs(const void *data, size_t length);

/* Memory formats */

struct cmd_capture_req
{
	char name[CAPTURE_FILENAME_LEN + 1];
};

struct cmd_get_length_req
{
	char name[CAPTURE_FILENAME_LEN + 1];
};

struct cmd_read_req
{
	size_t offset;
	char name[CAPTURE_FILENAME_LEN + 1];
};

struct cmd_capture_res
{
};

struct cmd_get_length_res
{
	size_t length;
};

struct cmd_read_res
{
	size_t size;
	CMD_READ_CS_TYPE cs;
	char data[CMD_READ_BLOCK_SIZE];
};

/* Wire formats */

struct __attribute__((__packed__)) wire_cmd_capture_req
{
	struct wire_cmd_req_header hdr;
	char name[CAPTURE_FILENAME_LEN];
};

struct __attribute__((__packed__)) wire_cmd_get_length_req
{
	struct wire_cmd_req_header hdr;
	char name[CAPTURE_FILENAME_LEN];
};

struct __attribute__((__packed__)) wire_cmd_read_req
{
	struct wire_cmd_req_header hdr;
	uint32_t offset;
	char name[CAPTURE_FILENAME_LEN];
};

struct __attribute__((__packed__)) wire_cmd_capture_res
{
	struct wire_cmd_res_header hdr;
};

struct __attribute__((__packed__)) wire_cmd_get_length_res
{
	struct wire_cmd_res_header hdr;
	uint32_t length;
};

struct __attribute__((__packed__)) wire_cmd_read_res
{
	struct wire_cmd_res_header hdr;
	CMD_READ_CS_TYPE cs;
	char data[CMD_READ_BLOCK_SIZE];
};
