#include "power.h"
#include "client.h"

bool payload_power_on(struct uart_context *uart)
{
	const time_t deadline = time(NULL) + PAYLOAD_POWER_ON_TIMEOUT_S - PAYLOAD_WARM_UP_TIME_S;
	loginfo("Powering payload 5V0 on");
	if (system("/usr/bin/iomgr/gpio high 39") != 0) {
		logfail("Failed to turn power line on");
		return false;
	}
	const time_t t0 = time(NULL);
	loginfo("Waiting for payload to respond");
	const unsigned total_pings = 5;
	unsigned pending = 0;
	do {
		if (time(NULL) > deadline) {
			logfail("Timed out waiting for payload to respond");
			return false;
		}
		if (cmd_ping(uart)) {
			pending++;
		} else {
			pending = 0;
		}
	} while (pending < total_pings);
	const time_t dt = time(NULL) - t0;
	loginfo("Payload took %us to power up", (unsigned) dt);
	loginfo("Waiting %us for sensor to warm up", PAYLOAD_WARM_UP_TIME_S);
	sleep(PAYLOAD_WARM_UP_TIME_S);
	loginfo("Ready");
	return true;
}

bool payload_power_off(struct uart_context *uart)
{
	const time_t deadline = time(NULL) + PAYLOAD_POWER_OFF_TIMEOUT_S;
	if (cmd_ping(uart)) {
		loginfo("Requesting payload to perform graceful shutdown");
		cmd_poweroff(uart);
		loginfo("Waiting for payload to shutdown");
		for (int i = 0; i < 10 && cmd_ping(uart); i++) {
			if (time(NULL) > deadline) {
				logfail("Timed out waiting for payload to shutdown - forcefully powering off");
				break;
			}
		}
		loginfo("Waiting for %us before powering payload off", PAYLOAD_COOL_DOWN_TIME_S);
		sleep(PAYLOAD_COOL_DOWN_TIME_S);
	}
	loginfo("Powering payload 5V0 off");
	if (system("/usr/bin/iomgr/gpio low 39") != 0) {
		logfail("Failed to turn power line off");
		return false;
	}
	return true;
}
