#pragma once
#include "../stdinc.h"
#include "../power/power.h"
#include "../uart/proto.h"

#define PAYLOAD_POWER_ON_TIMEOUT_S 90
#define PAYLOAD_POWER_OFF_TIMEOUT_S 60
#define PAYLOAD_WARM_UP_TIME_S 10
#define PAYLOAD_COOL_DOWN_TIME_S 10

bool payload_power_on(struct uart_context *uart);
bool payload_power_off(struct uart_context *uart);
