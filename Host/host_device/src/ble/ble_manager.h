/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

/* SinglePing Service UUID: 12345678-1234-1234-1234-123456789abc */
#define SINGLEPING_SERVICE_UUID \
    BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x1234, 0x1234, 0x123456789abc))

/* Ping Request Characteristic UUID: 12345678-1234-1234-1234-123456789abd */
#define PING_REQUEST_CHAR_UUID \
    BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x1234, 0x1234, 0x123456789abd))

/* Ping Response Characteristic UUID: 12345678-1234-1234-1234-123456789abe */
#define PING_RESPONSE_CHAR_UUID \
    BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x1234, 0x1234, 0x123456789abe))

/* Connection status callback type */
typedef void (*connection_status_cb_t)(bool connected);

/* Ping response callback type */
typedef void (*ping_response_cb_t)(const uint8_t *data, uint16_t len);

/**
 * @brief Initialize BLE manager
 * 
 * @param conn_cb Connection status callback
 * @return 0 on success, negative error code on failure
 */
int ble_manager_init(connection_status_cb_t conn_cb);

/**
 * @brief Start BLE scanning for MIPE devices
 * 
 * @return 0 on success, negative error code on failure
 */
int ble_manager_start_scan(void);

/**
 * @brief Stop BLE scanning
 * 
 * @return 0 on success, negative error code on failure
 */
int ble_manager_stop_scan(void);

/**
 * @brief Send ping request to connected MIPE device
 * 
 * @param data Ping request data
 * @param len Length of ping request data
 * @return 0 on success, negative error code on failure
 */
int ble_manager_send_ping_request(const uint8_t *data, uint16_t len);

/**
 * @brief Set ping response callback
 * 
 * @param cb Ping response callback
 */
void ble_manager_set_ping_response_callback(ping_response_cb_t cb);

/**
 * @brief Get current connection handle
 * 
 * @return Connection handle or NULL if not connected
 */
struct bt_conn *ble_manager_get_connection(void);

/**
 * @brief Check if BLE is connected
 * 
 * @return true if connected, false otherwise
 */
bool ble_manager_is_connected(void);

#endif /* BLE_MANAGER_H */
