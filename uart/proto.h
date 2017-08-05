#pragma once
#include "../stdinc.h"

/*
 * UART protocol handler, uses variable-length packets with sync word and length
 * at start to specify whether packet is start/middle/end of sequence.
 *
 * Packet is followed by short interframe in order to prevent entire next packet
 * from being lost in the event that a few bytes are skipped in a previous
 * packet.
 *
 * Checksums/FEC and length-trimming are expected to be handled in a higher
 * layer, if required.
 */

#define UART_PACKET_SIZE 4096
#define UART_SYNC_WORD_TYPE uint32_t
#define UART_SYNC_WORD_SINGLE ((UART_SYNC_WORD_TYPE) 0xd6276dc7UL)
#define UART_SYNC_WORD_START ((UART_SYNC_WORD_TYPE) 0x1cd026b1UL)
#define UART_SYNC_WORD_CONTINUE ((UART_SYNC_WORD_TYPE) 0xc949c3fcUL)
#define UART_SYNC_WORD_END 0x7138b6ad
#define UART_INTERFRAME_SIZE 10
#define UART_INTERFRAME_BYTE 0xaa

#define UART_FRAME_SIZE (UART_PACKET_SIZE + sizeof(UART_SYNC_WORD_TYPE) + UART_INTERFRAME_SIZE)

enum uart_packet_type
{
	pt_continue = 0x00,
	pt_start = 0x01,
	pt_end = 0x02,
	pt_single = pt_start | pt_end
};

enum uart_rx_stage
{
	urs_sync,
	urs_size,
	urs_data
};

struct uart_packet
{
	/* Bytes of data in buffer */
	size_t size;
	/* Packet type */
	enum uart_packet_type type;
	/* Data */
	char data[UART_PACKET_SIZE];
};

struct uart_context
{
	/* Internals */
	struct {
		enum uart_rx_stage stage;
		/* Bytes received of current packet (or of size words) */
		size_t len;
		/* Packet data */
		struct uart_packet packet;
	} rx_info;
	struct {
		UART_SYNC_WORD_TYPE sync;
		uint32_t size;
		unsigned size_bytes;
	} rx_wire;
	/* Called to transmit a byte */
	void (*on_tx_byte)(struct uart_context *ctx, uint8_t byte);
	/*
	 * Optional, called when a packet has been received (for asynchronous
	 * usage).
	 */
	void (*on_rx_packet)(struct uart_context *ctx, const struct uart_packet *packet);
	/*
	 * Optional, function to synchronously receive a packet (blocking
	 * until on_rx_packet has been triggered).
	 *
	 * This function is not required by the UART protocol library, it is
	 * intended for consumers of this class instead via uart_rx_packet.
	 *
	 * Is intended to be a function something like:
	 *
	 * bool rx_pull(struct uart_context *ctx)
	 * {
	 *   uint8_t b;
	 *   while (uart_receive_byte(&b)) {
	 *     if (uart_on_rx_byte(ctx, b)) {
	 *       return true;
	 *     }
	 *   }
	 *   return false;
	 * }
	 *
	 * Should return false if failed.
	 */
	bool (*on_rx_pull)(struct uart_context *ctx);
	/* User-defined */
	void *extra;
};

/*
 * Call when byte is received.
 *
 * Returns ctx->rx_buf on end of packet.
 */
const struct uart_packet *uart_on_rx_byte(struct uart_context *ctx, uint8_t byte);

/*
 * Call to transmit a buffer.
 *
 * Fragments data into multiple packets if needed.
 */
void uart_tx_data(struct uart_context *ctx, const void *data, size_t length);

/*
 * Blocking, uses ctx->on_rx_pull.
 *
 * Returns ctx->rx_buf on success.
 *
 * Returns NULL if failed or if no complete packet was received.
 *
 * Will only return the latest packet received, if multiple packets were
 * received during the operation.  Hence, the on_rx_pull function should return
 * true if uart_on_rx_byte returns non-NULL.
 */
const struct uart_packet *uart_rx_packet(struct uart_context *ctx);
