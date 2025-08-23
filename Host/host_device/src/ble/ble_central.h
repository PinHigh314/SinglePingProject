/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BLE_CENTRAL_H_
#define BLE_CENTRAL_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Callback for Mipe connection state changes
 * @param connected true if connected, false if disconnected
 */
typedef void (*mipe_connection_cb_t)(bool connected);

/**
 * @brief Callback for Mipe RSSI measurements
 * @param rssi RSSI value in dBm
 * @param timestamp Timestamp of the measurement
 */
typedef void (*mipe_rssi_cb_t)(int8_t rssi, uint32_t timestamp);

/**
 * @brief Initialize BLE central functionality
 * @param conn_cb Callback for connection state changes
 * @param rssi_cb Callback for RSSI measurements
 * @return 0 on success, negative error code on failure
 */
int ble_central_init(mipe_connection_cb_t conn_cb, mipe_rssi_cb_t rssi_cb);

/**
 * @brief Start scanning for Mipe devices
 * @return 0 on success, negative error code on failure
 */
int ble_central_start_scan(void);

/**
 * @brief Stop scanning for Mipe devices
 * @return 0 on success, negative error code on failure
 */
int ble_central_stop_scan(void);

/**
 * @brief Disconnect from Mipe device
 * @return 0 on success, negative error code on failure
 */
int ble_central_disconnect_mipe(void);

/**
 * @brief Request RSSI measurement from connected Mipe
 * @return 0 on success, negative error code on failure
 */
int ble_central_request_rssi(void);

/**
 * @brief Check if connected to Mipe device
 * @return true if connected, false otherwise
 */
bool ble_central_is_connected(void);

/**
 * @brief Check if scanning for Mipe devices
 * @return true if scanning, false otherwise
 */
bool ble_central_is_scanning(void);

#endif /* BLE_CENTRAL_H_ */
