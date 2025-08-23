#ifndef PING_SERVICE_H
#define PING_SERVICE_H

#include <stdint.h>

typedef void (*ping_request_cb_t)(const uint8_t *data, uint16_t len);

int ping_service_init(ping_request_cb_t request_cb);
int ping_service_send_response(const uint8_t *data, uint16_t len);
int ping_service_update_battery_voltage(uint16_t voltage_mv);

#endif
