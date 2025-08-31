#ifndef APP_CONNECTION_H
#define APP_CONNECTION_H

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <errno.h>
#include <stdbool.h>

// ========================================
// APP CONNECTION CONFIGURATION
// ========================================

// Device name matching App expectations
#define APP_DEVICE_NAME      "MIPE_HOST_A1B2"

// ========================================
// FUNCTION PROTOTYPES
// ========================================

/**
 * Initialize App connection (BLE peripheral)
 * @return 0 on success, negative error code on failure
 */
int app_connection_init(void);

/**
 * Start advertising to App
 * @return 0 on success, negative error code on failure
 */
int app_connection_start_advertising(void);

/**
 * Stop advertising to App
 * @return 0 on success, negative error code on failure
 */
int app_connection_stop_advertising(void);

/**
 * Check if App is connected
 * @return true if connected, false otherwise
 */
bool app_connection_is_connected(void);

/**
 * Get App connection object
 * @return Pointer to connection object, or NULL if not connected
 */
struct bt_conn *app_connection_get_conn(void);

/**
 * Disconnect from App
 * @return 0 on success, negative error code on failure
 */
int app_connection_disconnect(void);

/**
 * Check if advertising is active
 * @return true if advertising, false otherwise
 */
bool app_connection_is_advertising(void);

#endif // APP_CONNECTION_H
