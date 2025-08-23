/**
 * Connection Manager Header - P003 Implementation
 * 
 * Provides BLE connection management for Mipe device
 * Handles connection states, recovery, and power optimization
 */

#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Initialize connection manager
 * Sets up BLE connection handling and state machine
 */
void connection_manager_init(void);

/**
 * Update connection manager
 * Handles state transitions and timeout management
 * Should be called periodically from main loop
 */
void connection_manager_update(void);

/**
 * Check if device is currently connected
 * @return true if connected to Host, false otherwise
 */
bool connection_manager_is_connected(void);

/**
 * Enable or disable auto-reconnect feature
 * @param enable true to enable auto-reconnect, false to disable
 */
void connection_manager_set_auto_reconnect(bool enable);

/**
 * Request connection parameter update
 * Used to optimize power consumption during different operation modes
 * 
 * @param interval_min Minimum connection interval (units of 1.25ms)
 * @param interval_max Maximum connection interval (units of 1.25ms)
 * @param latency Connection latency (number of events)
 * @param timeout Supervision timeout (units of 10ms)
 * @return 0 on success, negative error code on failure
 */
int connection_manager_update_params(uint16_t interval_min, uint16_t interval_max,
                                     uint16_t latency, uint16_t timeout);

#endif // CONNECTION_MANAGER_H
