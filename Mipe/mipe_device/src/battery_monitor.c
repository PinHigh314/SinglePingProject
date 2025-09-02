/**
 * Mipe Device Battery Monitor Implementation
 * 
 * Critical for achieving 30+ day battery life target
 * Monitors battery voltage and reports status to Host
 */

#include "battery_monitor.h"
#include "led_control.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(battery_monitor, LOG_LEVEL_INF);

/* Battery voltage thresholds (in millivolts) */
#define BATTERY_VOLTAGE_MAX_MV     3300  /* 3.3V - Full battery */
#define BATTERY_VOLTAGE_MIN_MV     2200  /* 2.2V - Empty battery */
#define BATTERY_VOLTAGE_LOW_MV     2500  /* 2.5V - Low battery warning */
#define BATTERY_VOLTAGE_CRITICAL_MV 2300  /* 2.3V - Critical battery */

/* ADC configuration for battery measurement */
#define ADC_NODE           DT_NODELABEL(adc)
#define ADC_RESOLUTION     12
#define ADC_GAIN           ADC_GAIN_1_4
#define ADC_REFERENCE      ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40)
#define ADC_CHANNEL_ID     0

/* Battery monitoring state */
static uint8_t current_battery_level = 100;
static uint16_t current_voltage_mv = BATTERY_VOLTAGE_MAX_MV;
static bool low_battery_warning_sent = false;
static bool critical_battery_warning_sent = false;
static bool adc_initialized = false;  /* Track if ADC has been initialized */

/* ADC device and configuration */
static const struct device *adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc));
static struct adc_channel_cfg channel_cfg = {
    .gain = ADC_GAIN,
    .reference = ADC_REFERENCE,
    .acquisition_time = ADC_ACQUISITION_TIME,
    .channel_id = ADC_CHANNEL_ID,
    .input_positive = 2,  /* AIN2 - P0.04 pin */
};

static struct adc_sequence sequence = {
    .channels = BIT(ADC_CHANNEL_ID),
    .buffer = NULL,
    .buffer_size = 0,
    .resolution = ADC_RESOLUTION,
};

/* Forward declarations */
static uint16_t read_battery_voltage(void);
static uint8_t voltage_to_percentage(uint16_t voltage_mv);

/**
 * Initialize battery monitoring
 * POWER OPTIMIZED: Does nothing at boot, ADC init deferred to first SW3 press
 */
void battery_monitor_init(void)
{
    /* POWER OPTIMIZATION: Skip ALL initialization at boot */
    /* ADC will be initialized on first SW3 button press */
    LOG_INF("Battery monitor: DEFERRED INIT (press SW3 to activate)");
}

/**
 * Initialize ADC on first use (lazy initialization)
 * Called internally when SW3 is first pressed
 */
static int battery_monitor_init_adc(void)
{
    int err;
    
    if (adc_initialized) {
        return 0;  /* Already initialized */
    }
    
    LOG_INF("========================================");
    LOG_INF("ADC INITIALIZATION (FIRST SW3 PRESS)");
    LOG_INF("========================================");
    
    /* Get ADC device */
    if (!device_is_ready(adc_dev)) {
        /* If ADC is not available, use simulated battery */
        LOG_WRN("ADC device not ready, using simulated battery level");
        LOG_WRN("Check device tree configuration for ADC");
        current_battery_level = 85; /* Simulate 85% battery */
        return -ENODEV;
    }
    
    LOG_INF("ADC device is ready for battery monitoring");
    
    /* Configure ADC channel */
    LOG_INF("Configuring ADC channel %d", ADC_CHANNEL_ID);
    LOG_INF("  - Gain: 1/4");
    LOG_INF("  - Reference: Internal (0.6V)");
    LOG_INF("  - Resolution: %d bits", ADC_RESOLUTION);
    LOG_INF("  - Input: AIN2 (P0.04)");
    
    err = adc_channel_setup(adc_dev, &channel_cfg);
    if (err < 0) {
        LOG_ERR("ADC channel setup failed with error code: %d", err);
        LOG_ERR("Falling back to simulated battery level");
        current_battery_level = 85; /* Fallback to simulated */
        return err;
    }
    
    LOG_INF("ADC channel configured successfully");
    LOG_INF("========================================");
    
    adc_initialized = true;
    return 0;
}

/**
 * Read battery voltage once (on-demand)
 * Power-optimized function to read battery only when requested
 */
void battery_monitor_read_once(void)
{
    /* Initialize ADC on first use (lazy initialization) */
    if (!adc_initialized) {
        int err = battery_monitor_init_adc();
        if (err < 0) {
            LOG_ERR("Failed to initialize ADC on first use");
            return;
        }
    }
    
    LOG_INF("========================================");
    LOG_INF("BATTERY READ REQUESTED (SW3 BUTTON)");
    LOG_INF("========================================");
    
    /* Read current battery voltage */
    uint16_t voltage_mv = read_battery_voltage();
    uint8_t new_level = voltage_to_percentage(voltage_mv);
    
    LOG_INF("Battery Status:");
    LOG_INF("  Voltage: %u mV", voltage_mv);
    LOG_INF("  Level: %u%%", new_level);
    LOG_INF("  Status: %s", 
            voltage_mv <= BATTERY_VOLTAGE_CRITICAL_MV ? "CRITICAL" :
            voltage_mv <= BATTERY_VOLTAGE_LOW_MV ? "LOW" :
            "NORMAL");
    
    /* Check if battery level changed significantly (>5%) */
    int level_change = (int)new_level - (int)current_battery_level;
    if (abs(level_change) > 5) {
        LOG_INF("  Change from last read: %+d%%", level_change);
    }
    
    /* Always update the values when read on-demand */
    current_battery_level = new_level;
    current_voltage_mv = voltage_mv;
    
    /* Notify BLE service of battery reading */
    extern int ble_service_notify_battery(void);
    ble_service_notify_battery();
    
    /* Check for low battery conditions */
    if (voltage_mv <= BATTERY_VOLTAGE_CRITICAL_MV) {
        if (!critical_battery_warning_sent) {
            LOG_ERR("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
            LOG_ERR("CRITICAL BATTERY WARNING");
            LOG_ERR("  Voltage: %u mV", voltage_mv);
            LOG_ERR("  Level: %u%%", new_level);
            LOG_ERR("  Action: Consider charging immediately");
            LOG_ERR("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
            
            /* Power optimization: Don't use LED patterns to save battery */
            /* led_set_pattern(LED_ID_ERROR, LED_PATTERN_ERROR); */
            critical_battery_warning_sent = true;
            
            /* Force deep sleep to preserve remaining battery */
            if (new_level < 5) {
                LOG_ERR("Battery critically low - entering deep sleep");
                /* TMT6 will implement deep sleep mode */
            }
        }
    } else if (voltage_mv <= BATTERY_VOLTAGE_LOW_MV) {
        if (!low_battery_warning_sent) {
            LOG_WRN("========================================");
            LOG_WRN("LOW BATTERY WARNING");
            LOG_WRN("  Voltage: %u mV", voltage_mv);
            LOG_WRN("  Level: %u%%", new_level);
            LOG_WRN("========================================");
            
            /* Power optimization: Don't use LED patterns to save battery */
            /* led_set_pattern(LED_ID_ERROR, LED_PATTERN_SLOW_BLINK); */
            low_battery_warning_sent = true;
        }
    } else {
        /* Battery recovered above warning thresholds */
        if (low_battery_warning_sent || critical_battery_warning_sent) {
            LOG_INF("========================================");
            LOG_INF("BATTERY LEVEL RECOVERED");
            LOG_INF("  Voltage: %u mV", voltage_mv);
            LOG_INF("  Level: %u%%", new_level);
            LOG_INF("  Status: Normal operation");
            LOG_INF("========================================");
            
            /* Power optimization: LED already off */
            /* led_set_pattern(LED_ID_ERROR, LED_PATTERN_OFF); */
            low_battery_warning_sent = false;
            critical_battery_warning_sent = false;
        }
    }
    
    LOG_INF("========================================");
}

/**
 * Update battery monitoring (DEPRECATED)
 * This function is kept for compatibility but does nothing for power optimization
 */
void battery_monitor_update(void)
{
    /* Power optimization: This function is now a no-op */
    /* Battery is only read on-demand via battery_monitor_read_once() */
}

/**
 * Get current battery level as percentage
 * @return Battery level 0-100%
 */
uint8_t battery_monitor_get_level(void)
{
    /* For TMT3, return simulated battery level if ADC not available */
    if (!adc_dev || !device_is_ready(adc_dev)) {
        /* Simulate gradual battery drain for testing */
        static uint32_t sim_counter = 0;
        static bool logged_once = false;
        sim_counter++;
        
        if (!logged_once) {
            LOG_WRN("Using simulated battery level (ADC not available)");
            logged_once = true;
        }
        
        /* Decrease by 1% every 100 calls (simulating discharge) */
        uint8_t simulated = 95 - (sim_counter / 100);
        if (simulated < 10) simulated = 10; /* Don't go below 10% in simulation */
        return simulated;
    }
    
    return current_battery_level;
}

/**
 * Get current battery voltage in millivolts
 * @return Battery voltage in mV
 */
uint16_t battery_monitor_get_voltage_mv(void)
{
    return current_voltage_mv;
}

/**
 * Check if battery is low
 * @return true if battery is below low threshold
 */
bool battery_monitor_is_low(void)
{
    return (current_voltage_mv <= BATTERY_VOLTAGE_LOW_MV);
}

/**
 * Check if battery is critical
 * @return true if battery is critically low
 */
bool battery_monitor_is_critical(void)
{
    return (current_voltage_mv <= BATTERY_VOLTAGE_CRITICAL_MV);
}

/* Internal functions */

/**
 * Read battery voltage from ADC
 * @return Voltage in millivolts
 */
static uint16_t read_battery_voltage(void)
{
    if (!adc_initialized || !adc_dev || !device_is_ready(adc_dev)) {
        /* Return simulated voltage if ADC not available */
        static bool logged_once = false;
        if (!logged_once) {
            LOG_WRN("ADC not available - returning simulated voltage");
            logged_once = true;
        }
        return BATTERY_VOLTAGE_MAX_MV - 200; /* Simulate 2.8V */
    }
    
    LOG_DBG("Reading ADC channel %d for battery voltage", ADC_CHANNEL_ID);
    
    int16_t buf;
    sequence.buffer = &buf;
    sequence.buffer_size = sizeof(buf);
    
    int err = adc_read(adc_dev, &sequence);
    if (err < 0) {
        LOG_ERR("ADC read failed with error code: %d", err);
        LOG_ERR("Returning last known voltage: %u mV", current_voltage_mv);
        return current_voltage_mv; /* Return last known value */
    }
    
    LOG_DBG("ADC raw value: %d", buf);
    
    /* Convert ADC value to millivolts */
    /* This calculation depends on your hardware design */
    /* Adjust based on voltage divider and reference voltage */
    int32_t val_mv = buf;
    adc_raw_to_millivolts(adc_ref_internal(adc_dev), 
                         ADC_GAIN, 
                         ADC_RESOLUTION, 
                         &val_mv);
    
    LOG_DBG("ADC converted to: %d mV (before divider correction)", val_mv);
    
    /* Apply voltage divider correction if needed */
    /* For 3.3V system - adjust this multiplier based on your actual voltage divider */
    /* If no voltage divider, comment out the multiplication */
    /* val_mv *= 2; */  /* Uncomment and adjust if using voltage divider */
    
    LOG_DBG("Final battery voltage: %d mV (direct measurement, no divider)", val_mv);
    
    return (uint16_t)val_mv;
}

/**
 * Convert battery voltage to percentage
 * @param voltage_mv Voltage in millivolts
 * @return Battery percentage 0-100%
 */
static uint8_t voltage_to_percentage(uint16_t voltage_mv)
{
    if (voltage_mv >= BATTERY_VOLTAGE_MAX_MV) {
        return 100;
    }
    if (voltage_mv <= BATTERY_VOLTAGE_MIN_MV) {
        return 0;
    }
    
    /* Linear interpolation between min and max */
    uint32_t range = BATTERY_VOLTAGE_MAX_MV - BATTERY_VOLTAGE_MIN_MV;
    uint32_t offset = voltage_mv - BATTERY_VOLTAGE_MIN_MV;
    uint8_t percentage = (uint8_t)((offset * 100) / range);
    
    return percentage;
}
