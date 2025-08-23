/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "ping_engine.h"
#include "ble_manager.h"

LOG_MODULE_REGISTER(ping_engine, LOG_LEVEL_INF);

/* Ping engine state */
static bool initialized = false;
static bool running = false;
static uint32_t ping_interval_ms = 1000; /* Default 1 second */
static uint32_t sequence_number = 0;
static int64_t last_ping_time = 0;
static int64_t ping_start_time = 0;
static bool waiting_for_response = false;

/* Statistics */
static uint32_t pings_sent = 0;
static uint32_t pings_received = 0;
static uint32_t pings_lost = 0;
static uint32_t total_ping_time_us = 0;
static uint32_t min_ping_time_us = UINT32_MAX;
static uint32_t max_ping_time_us = 0;

/* Callback */
static ping_result_cb_t result_callback = NULL;

/* Timeout for ping response */
#define PING_TIMEOUT_MS 5000

/* Forward declarations */
static void ping_response_handler(const uint8_t *data, uint16_t len);
static uint64_t get_timestamp_us(void);

int ping_engine_init(ping_result_cb_t result_cb)
{
    LOG_INF("Initializing ping engine");

    if (initialized) {
        LOG_WRN("Ping engine already initialized");
        return 0;
    }

    result_callback = result_cb;

    /* Set up BLE ping response callback */
    ble_manager_set_ping_response_callback(ping_response_handler);

    /* Reset statistics */
    ping_engine_reset_stats();

    initialized = true;
    LOG_INF("Ping engine initialized");
    return 0;
}

int ping_engine_start(void)
{
    if (!initialized) {
        LOG_ERR("Ping engine not initialized");
        return -EINVAL;
    }

    if (running) {
        LOG_WRN("Ping engine already running");
        return 0;
    }

    LOG_INF("Starting ping engine");
    running = true;
    last_ping_time = 0; /* Force immediate first ping */
    waiting_for_response = false;

    return 0;
}

int ping_engine_stop(void)
{
    if (!running) {
        LOG_WRN("Ping engine not running");
        return 0;
    }

    LOG_INF("Stopping ping engine");
    running = false;
    waiting_for_response = false;

    return 0;
}

void ping_engine_process(void)
{
    int64_t current_time;
    struct ping_packet packet;
    int err;

    if (!initialized || !running) {
        return;
    }

    current_time = k_uptime_get();

    /* Check for ping timeout */
    if (waiting_for_response && 
        (current_time - ping_start_time) > PING_TIMEOUT_MS) {
        LOG_WRN("Ping timeout (seq: %u)", sequence_number - 1);
        
        pings_lost++;
        waiting_for_response = false;
        
        if (result_callback) {
            result_callback(0, false);
        }
    }

    /* Check if it's time to send next ping */
    if (!waiting_for_response && 
        (current_time - last_ping_time) >= ping_interval_ms) {
        
        /* Prepare ping packet */
        packet.sequence_number = sequence_number++;
        packet.timestamp_us = get_timestamp_us();
        
        /* Fill payload with pattern */
        for (int i = 0; i < sizeof(packet.payload); i++) {
            packet.payload[i] = (uint8_t)(0xAA + i);
        }

        LOG_DBG("Sending ping (seq: %u)", packet.sequence_number);

        /* Send ping request */
        err = ble_manager_send_ping_request((uint8_t *)&packet, sizeof(packet));
        if (err) {
            LOG_ERR("Failed to send ping request: %d", err);
            return;
        }

        ping_start_time = current_time;
        last_ping_time = current_time;
        waiting_for_response = true;
        pings_sent++;
    }
}

void ping_engine_set_interval(uint32_t interval_ms)
{
    if (interval_ms < 100) {
        interval_ms = 100; /* Minimum 100ms */
    } else if (interval_ms > 10000) {
        interval_ms = 10000; /* Maximum 10s */
    }

    ping_interval_ms = interval_ms;
    LOG_INF("Ping interval set to %u ms", ping_interval_ms);
}

uint32_t ping_engine_get_interval(void)
{
    return ping_interval_ms;
}

void ping_engine_get_stats(uint32_t *sent, uint32_t *received, uint32_t *lost,
                          uint32_t *avg_time_us, uint32_t *min_time_us, uint32_t *max_time_us)
{
    if (sent) *sent = pings_sent;
    if (received) *received = pings_received;
    if (lost) *lost = pings_lost;
    
    if (avg_time_us) {
        *avg_time_us = (pings_received > 0) ? 
                       (total_ping_time_us / pings_received) : 0;
    }
    
    if (min_time_us) {
        *min_time_us = (min_ping_time_us != UINT32_MAX) ? min_ping_time_us : 0;
    }
    
    if (max_time_us) *max_time_us = max_ping_time_us;
}

void ping_engine_reset_stats(void)
{
    pings_sent = 0;
    pings_received = 0;
    pings_lost = 0;
    total_ping_time_us = 0;
    min_ping_time_us = UINT32_MAX;
    max_ping_time_us = 0;
    sequence_number = 0;
    
    LOG_INF("Ping statistics reset");
}

static void ping_response_handler(const uint8_t *data, uint16_t len)
{
    struct ping_packet *response;
    uint64_t current_time_us;
    uint32_t ping_time_us;

    if (!waiting_for_response) {
        LOG_WRN("Received unexpected ping response");
        return;
    }

    if (len != sizeof(struct ping_packet)) {
        LOG_ERR("Invalid ping response length: %d", len);
        return;
    }

    response = (struct ping_packet *)data;
    current_time_us = get_timestamp_us();
    ping_time_us = (uint32_t)(current_time_us - response->timestamp_us);

    LOG_DBG("Ping response received (seq: %u, time: %u us)", 
            response->sequence_number, ping_time_us);

    /* Validate sequence number */
    if (response->sequence_number != (sequence_number - 1)) {
        LOG_WRN("Sequence number mismatch: expected %u, got %u",
                sequence_number - 1, response->sequence_number);
    }

    /* Update statistics */
    pings_received++;
    total_ping_time_us += ping_time_us;
    
    if (ping_time_us < min_ping_time_us) {
        min_ping_time_us = ping_time_us;
    }
    
    if (ping_time_us > max_ping_time_us) {
        max_ping_time_us = ping_time_us;
    }

    waiting_for_response = false;

    /* Notify application */
    if (result_callback) {
        result_callback(ping_time_us, true);
    }
}

static uint64_t get_timestamp_us(void)
{
    return k_uptime_get() * 1000ULL; /* Convert ms to us */
}
