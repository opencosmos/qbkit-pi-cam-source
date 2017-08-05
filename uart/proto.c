#include "proto.h"

static void rx_goto(struct uart_context *ctx, enum uart_rx_stage stage)
{
	switch (stage) {
	case urs_sync:
		ctx->rx_wire.sync = 0;
		ctx->rx_info.packet.type = -1;
		break;
	case urs_size:
		ctx->rx_wire.size = 0;
		ctx->rx_wire.size_bytes = 0;
		ctx->rx_info.packet.size = 0;
		break;
	case urs_data:
		ctx->rx_info.len = 0;
		ctx->rx_wire.size_bytes = 0;
		memset(ctx->rx_info.packet.data, 0, UART_PACKET_SIZE);
		break;
	}
	ctx->rx_info.stage = stage;
}

static void rx_sync(struct uart_context *ctx, uint8_t byte)
{
	ctx->rx_wire.sync <<= 8;
	ctx->rx_wire.sync |= byte;
	if (ctx->rx_wire.sync == UART_SYNC_WORD_SINGLE) {
		ctx->rx_info.packet.type = pt_single;
	} else if (ctx->rx_wire.sync == UART_SYNC_WORD_START) {
		ctx->rx_info.packet.type = pt_start;
	} else if (ctx->rx_wire.sync == UART_SYNC_WORD_CONTINUE) {
		ctx->rx_info.packet.type = pt_continue;
	} else if (ctx->rx_wire.sync == UART_SYNC_WORD_END) {
		ctx->rx_info.packet.type = pt_end;
	} else {
		return;
	}
	rx_goto(ctx, urs_size);
}

static void rx_size(struct uart_context *ctx, uint8_t byte)
{
	ctx->rx_wire.size <<= 8;
	ctx->rx_wire.size |= byte;
	ctx->rx_wire.size_bytes++;
	if (ctx->rx_wire.size_bytes != sizeof(ctx->rx_wire.size)) {
		return;
	}
	if (ctx->rx_wire.size == 0 || ctx->rx_wire.size > UART_PACKET_SIZE) {
		rx_goto(ctx, urs_sync);
		logfail("Invalid packet length (%zu)", (size_t) ctx->rx_wire.size);
		return;
	}
	ctx->rx_info.packet.size = ctx->rx_wire.size;
	rx_goto(ctx, urs_data);
}

static bool rx_data(struct uart_context *ctx, uint8_t byte)
{
	ctx->rx_info.packet.data[ctx->rx_info.len] = byte;
	ctx->rx_info.len++;
	if (ctx->rx_info.len != ctx->rx_info.packet.size) {
		return false;
	}
	rx_goto(ctx, urs_sync);
	if (ctx->on_rx_packet) {
		ctx->on_rx_packet(ctx, &ctx->rx_info.packet);
	}
	return true;
}

const struct uart_packet *uart_on_rx_byte(struct uart_context *ctx, uint8_t byte)
{
	switch (ctx->rx_info.stage) {
	case urs_sync:
		rx_sync(ctx, byte);
		break;
	case urs_size:
		rx_size(ctx, byte);
		break;
	case urs_data:
		if (rx_data(ctx, byte)) {
			return &ctx->rx_info.packet;
		}
		break;
	}
	return NULL;
}

static void uart_write_byte(struct uart_context *ctx, uint8_t byte)
{
	ctx->on_tx_byte(ctx, byte);
}

static void uart_write(struct uart_context *ctx, const void *data, size_t length)
{
	for (const uint8_t *it = data, *end = data + length; it != end; ++it) {
		uart_write_byte(ctx, *it);
	}
}

static void uart_write_repeat_byte(struct uart_context *ctx, uint8_t byte, size_t length)
{
	for (size_t i = 0; i < length; ++i) {
		uart_write_byte(ctx, byte);
	}
}

static void uart_write_u32(struct uart_context *ctx, UART_SYNC_WORD_TYPE word)
{
	for (unsigned i = 0; i < sizeof(word); ++i) {
		uint8_t byte = word >> (8 * (sizeof(word) - 1));
		word <<= 8;
		uart_write_byte(ctx, byte);
	}
}

void uart_tx_data(struct uart_context *ctx, const void *data, size_t length)
{
	const uint8_t *it = data;
	const uint8_t *end = data + length;
	UART_SYNC_WORD_TYPE sync = length <= UART_PACKET_SIZE ? UART_SYNC_WORD_SINGLE : UART_SYNC_WORD_START;
	const bool fragment = length > UART_PACKET_SIZE;
	while (it != end) {
		const size_t remaining = end - it;
		const bool partial = remaining <= UART_PACKET_SIZE;
		const size_t packet_len = partial ? remaining : UART_PACKET_SIZE;
		if (fragment && partial) {
			sync = UART_SYNC_WORD_END;
		}
		uart_write_u32(ctx, sync);
		uart_write_u32(ctx, packet_len);
		uart_write(ctx, it, packet_len);
		uart_write_repeat_byte(ctx, UART_INTERFRAME_BYTE, UART_INTERFRAME_SIZE);
		it += packet_len;
		sync = UART_SYNC_WORD_CONTINUE;
	}
}

const struct uart_packet *uart_rx_packet(struct uart_context *ctx)
{
	if (!ctx->on_rx_pull) {
		return NULL;
	}
	if (!ctx->on_rx_pull(ctx)) {
		return NULL;
	}
	return &ctx->rx_info.packet;
}
