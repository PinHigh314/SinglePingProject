/**
 * TestMipe - Minimal BLE Peripheral for Testing
 * 
 * Features:
 * - Advertises as "MIPE"
 * - Accepts all connection requests
 * - LED1 flashes 50ms during advertising
 * - LED1 solid when connected
 * - Returns to advertising when disconnected
 * - No other services or characteristics
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(testmipe, LOG_LEVEL_INF);

/* LED definitions - adjust for your board */
#define LED1_NODE DT_ALIAS(led1)
#if !DT_NODE_HAS_STATUS(LED1_NODE, okay)
#error "Unsupported board: led1 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

/* Connection state */
static struct bt_conn *current_conn = NULL;
static volatile bool is_connected = false;
static volatile bool advertising_active = false;

/* Forward declarations */
static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);

/* Connection callbacks */
BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

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
        return ret;
    }
    
    LOG_INF("LED initialized");
    return 0;
}

/**
 * Set LED state
 */
static void led_set(bool state)
{
    gpio_pin_set_dt(&led1, state);
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
    
    LOG_INF("Starting TestMipe - Minimal BLE Peripheral");
    
    /* Initialize LED */
    err = led_init();
    if (err) {
        LOG_ERR("LED init failed: %d", err);
        return err;
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
    }
    
    return 0;
}
