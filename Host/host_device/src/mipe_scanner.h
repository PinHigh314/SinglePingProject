#ifndef MIPE_SCANNER_H
#define MIPE_SCANNER_H

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

// ========================================
// MIPE SCANNER CONFIGURATION
// ========================================

// RSSI range for Mipe devices (-30 to -80 dBm as per App requirements)
#define MIPE_RSSI_MIN        -80
#define MIPE_RSSI_MAX        -30

// Mipe device name to scan for
#define MIPE_DEVICE_NAME     "MIPE"

// ========================================
// FUNCTION PROTOTYPES
// ========================================

/**
 * Initialize Mipe scanner
 * @return 0 on success, negative error code on failure
 */
int mipe_scanner_init(void);

/**
 * Start scanning for Mipe devices
 * @return 0 on success, negative error code on failure
 */
int mipe_scanner_start(void);

/**
 * Stop scanning for Mipe devices
 * @return 0 on success, negative error code on failure
 */
int mipe_scanner_stop(void);

/**
 * Check if scanner is active
 * @return true if scanning, false otherwise
 */
bool mipe_scanner_is_active(void);

/**
 * Connect to Mipe device for battery reading
 * @param addr Mipe device address
 * @return 0 on success, negative error code on failure
 */
int mipe_scanner_connect_to_mipe(const bt_addr_le_t *addr);

/**
 * Disconnect from Mipe device
 * @return 0 on success, negative error code on failure
 */
int mipe_scanner_disconnect_from_mipe(void);

/**
 * Check if connected to Mipe
 * @return true if connected, false otherwise
 */
bool mipe_scanner_is_connected_to_mipe(void);

/**
 * Read battery voltage from Mipe (fake 3.8V for now)
 * @param battery_voltage Pointer to store battery voltage
 * @return 0 on success, negative error code on failure
 */
int mipe_scanner_read_battery(float *battery_voltage);

/**
 * Get last known RSSI from Mipe
 * @return Last RSSI value, or 0 if no data available
 */
int8_t mipe_scanner_get_last_rssi(void);

/**
 * Get Mipe device address
 * @param addr Pointer to store device address
 * @return 0 on success, negative error code on failure
 */
int mipe_scanner_get_mipe_address(bt_addr_le_t *addr);

#endif // MIPE_SCANNER_H
