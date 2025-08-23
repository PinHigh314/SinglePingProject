#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <stdint.h>

/**
 * Initialize the battery voltage monitoring
 * @return 0 on success, negative error code on failure
 */
int battery_monitor_init(void);

/**
 * Get the current battery voltage
 * @return Battery voltage in millivolts
 */
uint16_t battery_monitor_get_voltage_mv(void);

/**
 * Start periodic battery voltage monitoring
 * @param period_ms Update period in milliseconds
 * @return 0 on success, negative error code on failure
 */
int battery_monitor_start(uint32_t period_ms);

/**
 * Stop periodic battery voltage monitoring
 */
void battery_monitor_stop(void);

/**
 * Update battery monitoring (called from main loop)
 */
void battery_monitor_update(void);

#endif /* BATTERY_MONITOR_H */
