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

#endif /* BLE_CENTRAL_H_ */
