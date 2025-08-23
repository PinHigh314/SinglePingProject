/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * BLE Peripheral header for v8 - Extended API
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BLE_PERIPHERAL_V8_H
#define BLE_PERIPHERAL_V8_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize BLE Peripheral with extended callbacks for v8
 * 
 * @param conn_cb Connection callback (called when MotoApp connects)
 * @param disconn_cb Disconnection callback (called when MotoApp disconnects)
 * @param stream_cb Streaming state callback (called when streaming starts/stops)
 * @param rssi_cb RSSI data callback (called to get RSSI data to transmit)
 * @return 0 on success, negative error code on failure
 */
int ble_peripheral_init(void (*conn_cb)(void), 
                       void (*disconn_cb)(void),
                       void (*stream_cb)(bool active),
                       int (*rssi_cb)(int8_t *rssi, uint32_t *timestamp));

/**
 * @brief Start BLE advertising
 * 
 * @return 0 on success, negative error code on failure
 */
int ble_peripheral_start_advertising(void);

/**
 * @brief Send RSSI data to MotoApp via BLE notification
 * 
 * @param rssi_value RSSI value in dBm (-128 to 0)
 * @param timestamp Timestamp in milliseconds
 * @return 0 on success, negative error code on failure
 */
int ble_peripheral_send_rssi_data(int8_t rssi_value, uint32_t timestamp);

/**
 * @brief Get current packet count
 * 
 * @return Number of packets sent
 */
uint32_t ble_peripheral_get_packet_count(void);

/**
 * @brief Check if MotoApp is connected
 * 
 * @return true if connected, false otherwise
 */
bool ble_peripheral_is_connected(void);

/**
 * @brief Check if data streaming is enabled
 * 
 * @return true if streaming, false otherwise
 */
bool ble_peripheral_is_streaming(void);

#endif /* BLE_PERIPHERAL_V8_H */
