/**
 * Mipe Device BLE Service Header
 * 
 * Power-optimized BLE Peripheral interface for Host communication
 */

#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include <stdbool.h>

/**
 * Initialize BLE service
 * Sets up Bluetooth stack, GATT services, and starts advertising
 */
void ble_service_init(void);

/**
 * Update BLE service
 * Called from main loop to process BLE events
 */
void ble_service_update(void);

/**
 * Start power-optimized listening mode
 * Enters low-power advertising mode after disconnection
 * for reconnection with minimal battery consumption
 */
void ble_service_start_listening_mode(void);

/**
 * Send battery level notification to Host
 * Only sends when battery level changes significantly
 * @return 0 on success, negative error code on failure
 */
int ble_service_notify_battery(void);

/**
 * Check if connected to Host
 * @return true if connected, false otherwise
 */
bool ble_service_is_connected(void);

#endif // BLE_SERVICE_H
