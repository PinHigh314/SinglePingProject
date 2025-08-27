/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BLE_CENTRAL_H_
#define BLE_CENTRAL_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Callback for Mipe RSSI measurements from advertising packets
 * @param rssi RSSI value in dBm
 * @param timestamp Timestamp of the measurement
 */
typedef void (*mipe_rssi_cb_t)(int8_t rssi, uint32_t timestamp);

/**
 * @brief Initialize BLE central functionality for beacon mode
 * @param rssi_cb Callback for RSSI measurements from advertising packets
 * @return 0 on success, negative error code on failure
 */
int ble_central_init(mipe_rssi_cb_t rssi_cb);

/**
 * @brief Start scanning for Mipe device advertising packets
 * @return 0 on success, negative error code on failure
 */
int ble_central_start_scan(void);

/**
 * @brief Stop scanning for Mipe devices
 * @return 0 on success, negative error code on failure
 */
int ble_central_stop_scan(void);

/**
 * @brief Check if scanning for Mipe devices
 * @return true if scanning, false otherwise
 */
bool ble_central_is_scanning(void);

/**
 * @brief Connect to a detected Mipe device
 * @param addr Address of the Mipe device to connect to
 * @return 0 on success, negative error code on failure
 */
int ble_central_connect_to_mipe(const bt_addr_le_t *addr);

/**
 * @brief Disconnect from Mipe device
 * @return 0 on success, negative error code on failure
 */
int ble_central_disconnect(void);

/**
 * @brief Check if connected to Mipe device
 * @return true if connected, false otherwise
 */
bool ble_central_is_connected(void);

/**
 * @brief Get the stored Mipe device address for connection
 * @param addr Pointer to store the device address
 * @return true if Mipe device address is available, false otherwise
 */
bool ble_central_get_mipe_address(bt_addr_le_t *addr);

/**
 * @brief Clear the stored Mipe device address
 */
void ble_central_clear_mipe_address(void);

#endif /* BLE_CENTRAL_H_ */
