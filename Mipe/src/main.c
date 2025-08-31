/**
 * TestMipe - Minimal BLE Peripheral for Testing
 * 
 * Features:
 * - Advertises as "MIPE"
 * - Accepts all connection requests
 * - LED1 flashes 50ms during advertising
 * - LED1 solid when connected
 * - Returns to advertising when disconnected
 * - Battery service with GATT characteristics
 * - Real battery voltage reading (ADC-based)
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(testmipe, LOG_LEVEL_INF);

/* LED definitions - adjust for your board */
#define LED1_NODE DT_ALIAS(led1)
#if !DT_NODE_HAS_STATUS(LED1_NODE, okay)
#error "Unsupported board: led1 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

/* ADC definitions for battery voltage reading */
#define ADC_NODE DT_ALIAS(adc0)
#if !DT_NODE_HAS_STATUS(ADC_NODE, okay)
#error "Unsupported board: adc0 devicetree alias is not defined"
#endif
static const struct adc_dt_spec adc = ADC_DT_SPEC_GET(ADC_NODE);

/* Battery service UUIDs */
#define BT_UUID_BATTERY_SERVICE BT_UUID_DECLARE_16(0x180F)
#define BT_UUID_BATTERY_LEVEL BT_UUID_DECLARE_16(0x2A19)

/* Connection state */
static struct bt_conn *current_conn = NULL;
static volatile bool is_connected = false;
static volatile bool advertising_active = false;

/* Battery level (0-100%) */
static uint8_t battery_level = 75; // Default 75%

/* Forward declarations */
static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static ssize_t read_battery_level(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset);

/* Connection callbacks */
BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

/* GATT Service Definition */
BT_GATT_SERVICE_DEFINE(battery_svc,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_BATTERY_SERVICE),
    BT_GATT_CHARACTERISTIC(BT_UUID_BATTERY_LEVEL,
                           BT_GATT_CHRC_READ,
                           BT_GATT_PERM_READ,
                           read_battery_level, NULL, NULL),
);

/* Advertising data - just device name */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, "MIPE", 4),
};

/* Advertising parameters - fast advertising (100ms intervals) */
static struct bt_le_adv_param adv_param = BT_LE_ADV_PARAM_INIT(
    BT_LE_ADV_OPT_CONN,
    BT_GAP_ADV_FAST_INT_MIN_2,  /* 100ms min */
    BT_GAP_ADV_FAST_INT_MAX_2,  /* 150ms max */
    NULL);

/**
 * Initialize LED control
 */
static int led_init(void)
{
    int ret;
    
    if (!gpio_is_ready_dt(&led1)) {
        LOG_ERR("LED device not ready");
        return -ENODEV;
    }
    
    ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure LED: %d", ret);
        return 0;
    }
    
    LOG_INF("LED initialized");
    return 0;
}

/**
 * Initialize ADC for battery voltage reading
 */
static int adc_init(void)
{
    int ret;
    
    if (!adc_is_ready_dt(&adc)) {
        LOG_ERR("ADC device not ready");
        return -ENODEV;
    }
    
    ret = adc_channel_setup_dt(&adc);
    if (ret < 0) {
        LOG_ERR("Failed to setup ADC channel: %d", ret);
        return ret;
    }
    
    LOG_INF("ADC initialized");
    return 0;
}

/**
 * Read battery voltage and convert to percentage
 */
static void read_battery_voltage(void)
{
    int ret;
    uint16_t buf;
    struct adc_sequence sequence = {
        .buffer = &buf,
        .buffer_size = sizeof(buf),
    };
    
    adc_sequence_init_dt(&adc, &sequence);
    
    ret = adc_read_dt(&adc, &sequence);
    if (ret < 0) {
        LOG_WRN("Failed to read ADC: %d", ret);
        return;
    }
    
    /* Convert ADC reading to voltage (assuming 3.3V reference) */
    /* This is a simplified conversion - adjust based on your voltage divider */
    uint32_t adc_voltage = (uint32_t)buf * 3300 / 4096; // 12-bit ADC
    
    /* Convert to battery percentage (assuming 3.0V = 0%, 4.2V = 100%) */
    if (adc_voltage < 3000) {
        battery_level = 0;
    } else if (adc_voltage > 4200) {
        battery_level = 100;
    } else {
        battery_level = (uint8_t)((adc_voltage - 3000) * 100 / 1200);
    }
    
    LOG_INF("Battery: %dmV (%d%%)", adc_voltage, battery_level);
}

/**
 * Set LED state
 */
static void led_set(bool state)
{
    gpio_pin_set_dt(&led1, state);
}

/**
 * GATT read callback for battery level
 */
static ssize_t read_battery_level(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset)
{
    uint8_t level = battery_level;
    
    LOG_INF("Battery level read request: %d%%", level);
    
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &level, sizeof(level));
}

/**
 * Connection established callback
 */
static void connected(struct bt_conn *conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    
    if (err) {
        LOG_ERR("Connection failed to %s (err %d)", addr, err);
        return;
    }
    
    LOG_INF("Connected: %s", addr);
    
    /* Store connection reference */
    current_conn = bt_conn_ref(conn);
    is_connected = true;
    
    /* LED solid when connected */
    led_set(true);
}

/**
 * Connection lost callback
 */
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Disconnected: %s (reason 0x%02x)", addr, reason);
    
    /* Clean up connection reference */
    if (current_conn) {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }
    
    is_connected = false;
    advertising_active = false;
    
    /* Small delay for clean transition */
    k_msleep(50);
    
    /* Restart advertising with proper error handling */
    int err = bt_le_adv_stop();
    if (err && err != -EALREADY) {
        LOG_WRN("Failed to stop advertising (err %d)", err);
    }
    
    k_msleep(50);
    
    err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("Failed to restart advertising (err %d)", err);
        /* Try again after a delay */
        k_msleep(1000);
        err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
        if (err) {
            LOG_ERR("Second attempt to restart advertising failed (err %d)", err);
        } else {
            advertising_active = true;
            LOG_INF("Advertising restarted (second attempt)");
        }
    } else {
        advertising_active = true;
        LOG_INF("Advertising restarted");
    }
}

/**
 * Main application entry point
 */
int main(void)
{
    int err;
    
    LOG_INF("Starting TestMipe - BLE Peripheral with Battery Service");
    
    /* Initialize LED */
    err = led_init();
    if (err) {
        LOG_ERR("LED init failed: %d", err);
        return err;
    }
    
    /* Initialize ADC */
    err = adc_init();
    if (err) {
        LOG_WRN("ADC init failed: %d (using default battery level)", err);
    }
    
    /* Initialize Bluetooth */
    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed: %d", err);
        return err;
    }
    
    LOG_INF("Bluetooth initialized");
    
    /* Start advertising */
    err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("Advertising failed to start: %d", err);
        return err;
    }
    
    advertising_active = true;
    LOG_INF("Advertising started - Device name: MIPE");
    
    /* Main control loop */
    while (1) {
        if (!is_connected) {
            if (!advertising_active) {
                /* Try to restart advertising if it stopped */
                int adv_err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
                if (adv_err) {
                    LOG_WRN("Advertising stopped, restart failed: %d", adv_err);
                    k_msleep(1000);
                } else {
                    advertising_active = true;
                    LOG_INF("Advertising restarted in main loop");
                }
            }
            
            /* Advertising mode - flash LED for 50ms */
            led_set(true);
            k_msleep(50);
            led_set(false);
            k_msleep(50);
        } else {
            /* Connected mode - LED stays solid */
            led_set(true);
            k_msleep(100);
        }
        
        /* Update battery level every 10 seconds */
        static uint32_t last_battery_read = 0;
        if (k_uptime_get() - last_battery_read > 10000) {
            read_battery_voltage();
            last_battery_read = k_uptime_get();
        }
    }
    
    return 0;
}
