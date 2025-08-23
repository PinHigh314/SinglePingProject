/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BLE_CENTRAL_H_
#define BLE_CENTRAL_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * BLE Central Module for Mipe Device Connection
 * 
 * This module handles:
 * - Scanning for Mipe devices
 * - Connecting to discovered Mipe devices
 * - Managing GATT operations with Mipe
 * - Collecting RSSI data from Mipe
 */

/* Callback types */
typedef void (*mipe_connection_cb_t)(bool connected);
typedef void (*mipe_rssi_cb_t)(int8_t rssi, uint32_t timestamp);

/**
 * Initialize BLE Central for Mipe device connections
 *
 * @param conn_cb Callback for Mipe connection state changes
 * @param rssi_cb Callback for RSSI data from Mipe
 * @return 0 on success, negative error code on failure
 */
int ble_central_init(mipe_connection_cb_t conn_cb, mipe_rssi_cb_t rssi_cb);

/**
 * Start scanning for Mipe devices
 *
 * @return 0 on success, negative error code on failure
 */
int ble_central_start_scan(void);

/**
 * Stop scanning for Mipe devices
 *
 * @return 0 on success, negative error code on failure
 */
int ble_central_stop_scan(void);

/**
 * Disconnect from current Mipe device
 *
 * @return 0 on success, negative error code on failure
 */
int ble_central_disconnect_mipe(void);

/**
 * Request RSSI measurement from connected Mipe
 *
 * @return 0 on success, negative error code on failure
 */
int ble_central_request_rssi(void);

/**
 * Check if connected to a Mipe device
 *
 * @return true if connected, false otherwise
 */
bool ble_central_is_connected(void);

#endif /* BLE_CENTRAL_H_ */
