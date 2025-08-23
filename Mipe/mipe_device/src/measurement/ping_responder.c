#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "ping_responder.h"
#include "../ble/ping_service.h"

LOG_MODULE_REGISTER(ping_responder, LOG_LEVEL_INF);

/* Ping response structure */
struct ping_response {
    uint32_t sequence;
    uint32_t timestamp;
    uint32_t response_time_us;
    uint8_t status;
} __packed;

/* Ping request structure */
struct ping_request {
    uint32_t sequence;
    uint32_t timestamp;
    uint16_t payload_size;
    uint8_t payload[];
} __packed;

static uint32_t ping_counter = 0;

int ping_responder_init(void)
{
    ping_counter = 0;
    LOG_INF("Ping responder initialized");
    return 0;
}

void ping_responder_process(void)
{
    /* This function can be used for periodic processing if needed */
    /* Currently no periodic processing required */
}

int ping_responder_handle_request(const uint8_t *data, uint16_t len)
{
    struct ping_request *request;
    struct ping_response response;
    int64_t current_time;
    int err;

    if (len < sizeof(struct ping_request)) {
        LOG_ERR("Invalid ping request size: %u", len);
        return -EINVAL;
    }

    request = (struct ping_request *)data;
    current_time = k_uptime_get();

    LOG_DBG("Processing ping request seq: %u, timestamp: %u, payload_size: %u",
            request->sequence, request->timestamp, request->payload_size);

    /* Prepare response */
    response.sequence = request->sequence;
    response.timestamp = request->timestamp;
    response.response_time_us = (uint32_t)(current_time * 1000); /* Convert to microseconds */
    response.status = 0; /* Success */

    ping_counter++;

    /* Send response via BLE service */
    err = ping_service_send_response((uint8_t *)&response, sizeof(response));
    if (err) {
        LOG_ERR("Failed to send ping response: %d", err);
        return err;
    }

    LOG_DBG("Ping response sent for seq: %u, response_time: %u us", 
            response.sequence, response.response_time_us);

    return 0;
}
