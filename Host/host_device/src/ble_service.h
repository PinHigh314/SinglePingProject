#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

// ========================================
// TMT1 SERVICE DEFINITIONS
// ========================================
// Matching the MotoApp expectations exactly

// TMT1 Service UUID
#define BT_UUID_TMT1_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)

// TMT1 Service Characteristics
#define BT_UUID_RSSI_DATA_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1)

#define BT_UUID_CONTROL_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef2)

#define BT_UUID_STATUS_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef3)

#define BT_UUID_MIPE_STATUS_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef4)

#define BT_UUID_LOG_DATA_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef5)

// UUID structs for GATT service definition
static const struct bt_uuid_128 tmt1_service_uuid = BT_UUID_INIT_128(BT_UUID_TMT1_SERVICE_VAL);
static const struct bt_uuid_128 rssi_data_uuid = BT_UUID_INIT_128(BT_UUID_RSSI_DATA_VAL);
static const struct bt_uuid_128 control_uuid = BT_UUID_INIT_128(BT_UUID_CONTROL_VAL);
static const struct bt_uuid_128 status_uuid = BT_UUID_INIT_128(BT_UUID_STATUS_VAL);
static const struct bt_uuid_128 mipe_status_uuid = BT_UUID_INIT_128(BT_UUID_MIPE_STATUS_VAL);
static const struct bt_uuid_128 log_data_uuid = BT_UUID_INIT_128(BT_UUID_LOG_DATA_VAL);

// Control Commands (matching App expectations)
#define CMD_START_STREAM    0x01
#define CMD_STOP_STREAM     0x02
#define CMD_GET_STATUS      0x03
#define CMD_MIPE_SYNC       0x04

// ========================================
// FUNCTION PROTOTYPES
// ========================================

/**
 * Initialize BLE service
 * @return 0 on success, negative error code on failure
 */
int ble_service_init(void);

/**
 * Check if App is connected
 * @return true if connected, false otherwise
 */
bool ble_service_is_app_connected(void);

/**
 * Send RSSI data to App
 * @param rssi RSSI value (-30 to -80 dBm)
 * @param timestamp Timestamp in milliseconds
 * @return 0 on success, negative error code on failure
 */
int ble_service_send_rssi_data(int8_t rssi, uint32_t timestamp);

/**
 * Send Mipe status to App
 * @param connection_state Connection state (0=Idle, 1=Scanning, 2=Connected, 3=Connected, 4=Disconnected)
 * @param rssi RSSI value during connection
 * @param device_address Mipe device address
 * @param connection_duration Connection duration in milliseconds
 * @param battery_voltage Battery voltage (fake 3.8V for now)
 * @return 0 on success, negative error code on failure
 */
int ble_service_send_mipe_status(uint8_t connection_state, int8_t rssi, 
                                const uint8_t *device_address, uint32_t connection_duration,
                                float battery_voltage);

/**
 * Send log data to App
 * @param log_string Log message string
 * @return 0 on success, negative error code on failure
 */
int ble_service_send_log_data(const char *log_string);

/**
 * Handle control command from App
 * @param data Control command data
 * @param len Length of data
 * @return 0 on success, negative error code on failure
 */
int ble_service_handle_control_command(const uint8_t *data, uint16_t len);

/**
 * Set App connection object
 * @param conn Connection object or NULL if disconnected
 */
void ble_service_set_app_conn(struct bt_conn *conn);

#endif // BLE_SERVICE_H
