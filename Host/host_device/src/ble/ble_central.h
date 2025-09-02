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
 * @brief Check if Mipe device is currently detected
 * @return true if Mipe beacon is detected, false otherwise
 */
bool ble_central_is_mipe_detected(void);

/**
 * @brief Get the number of packets received from Mipe
 * @return Number of packets received since detection
 */
uint32_t ble_central_get_mipe_packet_count(void);

/**
 * @brief Get the battery voltage from Mipe's advertising data
 * @return Battery voltage in millivolts, 0 if not available
 */
uint16_t ble_central_get_mipe_battery_mv(void);

#endif /* BLE_CENTRAL_H_ */
