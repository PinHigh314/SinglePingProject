/**
 * BLE Service Header for Host Device - Simplified Working Version
 * 
 * This header provides basic BLE central functionality:
 * - Scanning for BLE devices
 * - Connecting to devices
 * - Basic BLE communication
 */

#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include <zephyr/bluetooth/conn.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize BLE service
 * 
 * @return 0 on success, negative error code on failure
 */
int ble_service_init(void);

/**
 * Check if connected to a BLE device
 * 
 * @return true if connected, false otherwise
 */
bool ble_service_is_connected(void);

/**
 * Get current BLE connection
 * 
 * @return Pointer to current connection, or NULL if not connected
 */
struct bt_conn *ble_service_get_connection(void);

/**
 * Disconnect from current BLE device
 */
void ble_service_disconnect(void);

/**
 * Start scanning for BLE devices
 * 
 * @return 0 on success, negative error code on failure
 */
int ble_service_start_scan(void);

/**
 * Stop scanning for BLE devices
 * 
 * @return 0 on success, negative error code on failure
 */
int ble_service_stop_scan(void);

#ifdef __cplusplus
}
#endif

#endif /* BLE_SERVICE_H */
