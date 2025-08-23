/**
 * Mipe Device Battery Monitor Header
 * 
 * Battery monitoring interface for power management
 */

#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Initialize battery monitoring
 * Sets up ADC for battery voltage measurement
 */
void battery_monitor_init(void);

/**
 * Update battery monitoring
 * Should be called periodically to check battery status
 */
void battery_monitor_update(void);

/**
 * Get current battery level as percentage
 * @return Battery level 0-100%
 */
uint8_t battery_monitor_get_level(void);

/**
 * Get current battery voltage in millivolts
 * @return Battery voltage in mV
 */
uint16_t battery_monitor_get_voltage_mv(void);

/**
 * Check if battery is low
 * @return true if battery is below low threshold (20%)
 */
bool battery_monitor_is_low(void);

/**
 * Check if battery is critical
 * @return true if battery is critically low (<10%)
 */
bool battery_monitor_is_critical(void);

#endif // BATTERY_MONITOR_H
