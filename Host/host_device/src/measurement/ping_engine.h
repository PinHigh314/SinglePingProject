/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PING_ENGINE_H
#define PING_ENGINE_H

#include <zephyr/kernel.h>

/* Ping result callback type */
typedef void (*ping_result_cb_t)(uint32_t ping_time_us, bool success);

/* Ping packet structure */
struct ping_packet {
    uint32_t sequence_number;
    uint64_t timestamp_us;
    uint8_t payload[16];
} __packed;

/**
 * @brief Initialize ping engine
 * 
 * @param result_cb Ping result callback
 * @return 0 on success, negative error code on failure
 */
int ping_engine_init(ping_result_cb_t result_cb);

/**
 * @brief Start ping measurements
 * 
 * @return 0 on success, negative error code on failure
 */
int ping_engine_start(void);

/**
 * @brief Stop ping measurements
 * 
 * @return 0 on success, negative error code on failure
 */
int ping_engine_stop(void);

/**
 * @brief Process ping engine (call from main loop)
 */
void ping_engine_process(void);

/**
 * @brief Set ping interval
 * 
 * @param interval_ms Ping interval in milliseconds
 */
void ping_engine_set_interval(uint32_t interval_ms);

/**
 * @brief Get current ping interval
 * 
 * @return Current ping interval in milliseconds
 */
uint32_t ping_engine_get_interval(void);

/**
 * @brief Get ping statistics
 * 
 * @param sent Number of pings sent
 * @param received Number of pings received
 * @param lost Number of pings lost
 * @param avg_time_us Average ping time in microseconds
 * @param min_time_us Minimum ping time in microseconds
 * @param max_time_us Maximum ping time in microseconds
 */
void ping_engine_get_stats(uint32_t *sent, uint32_t *received, uint32_t *lost,
                          uint32_t *avg_time_us, uint32_t *min_time_us, uint32_t *max_time_us);

/**
 * @brief Reset ping statistics
 */
void ping_engine_reset_stats(void);

#endif /* PING_ENGINE_H */
