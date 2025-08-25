/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BLE_PERIPHERAL_H_
#define BLE_PERIPHERAL_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Structure for Mipe status data
 */
typedef struct {
    uint8_t status_flags;
    int8_t rssi;
    uint32_t last_scan_timestamp;
    uint16_t connection_attempts;
    float battery_voltage;
    uint16_t connection_duration;
    char connection_state[16];
    char device_address[18];
} mipe_status_t;

/**
 * @brief Callback function for connection established
 */
typedef void (*app_connected_cb_t)(void);

/**
 * @brief Callback function for connection lost
 */
typedef void (*app_disconnected_cb_t)(void);

/**
 * @brief Callback function for streaming state changes
 * @param active true if streaming is active, false otherwise
 */
typedef void (*streaming_state_cb_t)(bool active);

/**
 * @brief Callback function to get RSSI data
 * @param rssi Pointer to store RSSI value
 * @param timestamp Pointer to store timestamp
 * @return 0 on success, negative error code on failure
 */
typedef int (*get_rssi_data_cb_t)(int8_t *rssi, uint32_t *timestamp);

/**
 * @brief Callback function for Mipe sync command
 */
typedef void (*mipe_sync_cb_t)(void);

/**
 * @brief Initialize BLE peripheral with callbacks
 * @param conn_cb Callback for connection events
 * @param disconn_cb Callback for disconnection events
 * @param stream_cb Callback for streaming state changes
 * @param rssi_cb Callback to get RSSI data
 * @param mipe_sync_cb Callback for Mipe sync command
 * @return 0 on success, negative error code on failure
 */
int ble_peripheral_init(app_connected_cb_t conn_cb, 
                       app_disconnected_cb_t disconn_cb,
                       streaming_state_cb_t stream_cb, 
                       get_rssi_data_cb_t rssi_cb,
                       mipe_sync_cb_t mipe_sync_cb);

/**
 * @brief Start BLE advertising
 * @return 0 on success, negative error code on failure
 */
int ble_peripheral_start_advertising(void);

/**
 * @brief Send RSSI data notification
 * @param rssi_value RSSI value to send
 * @param timestamp Timestamp of the measurement
 * @return 0 on success, negative error code on failure
 */
int ble_peripheral_send_rssi_data(int8_t rssi_value, uint32_t timestamp);

/**
 * @brief Get the current packet count
 * @return Number of packets sent
 */
uint32_t ble_peripheral_get_packet_count(void);

/**
 * @brief Check if peripheral is connected
 * @return true if connected, false otherwise
 */
bool ble_peripheral_is_connected(void);

/**
 * @brief Check if streaming is active
 * @return true if streaming, false otherwise
 */
bool ble_peripheral_is_streaming(void);

/**
 * @brief Update Mipe status characteristic
 * @param status Pointer to the Mipe status data
 * @return 0 on success, negative error code on failure
 */
int ble_peripheral_update_mipe_status(const mipe_status_t *status);

/**
 * @brief Send log data notification
 * @param log_str The log string to send
 * @return 0 on success, or a negative error code on failure
 */
int ble_peripheral_send_log_data(const char *log_str);

#endif /* BLE_PERIPHERAL_H_ */
