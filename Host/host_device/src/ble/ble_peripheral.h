/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * TMT1 BLE Peripheral header for MotoApp communication
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BLE_PERIPHERAL_H
#define BLE_PERIPHERAL_H

#include <stdint.h>
#include <stdbool.h>

/* Callback types */
typedef void (*ble_connection_cb_t)(bool connected);
typedef void (*data_stream_cb_t)(bool start);

/**
 * @brief Initialize BLE Peripheral for TMT1
 * 
 * @param conn_cb Connection status callback
 * @param stream_cb Data stream control callback
 * @return 0 on success, negative error code on failure
 */
int ble_peripheral_init(ble_connection_cb_t conn_cb, data_stream_cb_t stream_cb);

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

#endif /* BLE_PERIPHERAL_H */
