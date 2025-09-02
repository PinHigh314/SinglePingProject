/**
 * MIPE Device BLE Service - Simplified from Host code
 * Based on proven working Host BLE peripheral implementation
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/byteorder.h>

#include "ble_service.h"
#include "led_control.h"
#include "battery_monitor.h"

LOG_MODULE_REGISTER(ble_service, LOG_LEVEL_INF);

/* MIPE Service UUID: 87654321-4321-8765-4321-987654321098 */
#define MIPE_SERVICE_UUID BT_UUID_DECLARE_128( \
    BT_UUID_128_ENCODE(0x87654321, 0x4321, 0x8765, 0x4321, 0x987654321098))

/* Battery Characteristic UUID: 87654323-4321-8765-4321-987654321098 */
#define BATTERY_CHAR_UUID BT_UUID_DECLARE_128( \
    BT_UUID_128_ENCODE(0x87654323, 0x4321, 0x8765, 0x4321, 0x987654321098))

/* Connection state */
static struct bt_conn *current_conn = NULL;
static bool battery_notify_enabled = false;

/* Data buffers */
static uint8_t battery_level = 100;   /* Battery percentage */

/* Forward declarations */
static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static ssize_t read_battery(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                            void *buf, uint16_t len, uint16_t offset);
static void battery_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value);

/* Connection callbacks */
BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

/* MIPE Service Definition - following Host pattern */
BT_GATT_SERVICE_DEFINE(mipe_service,
    BT_GATT_PRIMARY_SERVICE(MIPE_SERVICE_UUID),
    
    /* Battery Characteristic - Read & Notify */
    BT_GATT_CHARACTERISTIC(BATTERY_CHAR_UUID,
                          BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                          BT_GATT_PERM_READ,
                          read_battery, NULL, &battery_level),
    BT_GATT_CCC(battery_ccc_changed,
               BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

/* Advertising data */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL,
                  BT_UUID_128_ENCODE(0x87654321, 0x4321, 0x8765, 0x4321, 0x987654321098)),
};

/* Scan response data with device name */
static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, "SinglePing Mipe", 15),
};

/* Advertising parameters - using Host's proven approach */
static struct bt_le_adv_param adv_param = BT_LE_ADV_PARAM_INIT(
    BT_LE_ADV_OPT_CONN,
    BT_GAP_ADV_FAST_INT_MIN_2,  /* 100ms */
    BT_GAP_ADV_FAST_INT_MAX_2,  /* 150ms */
    NULL);

void ble_service_init(void)
{
    int err;

    LOG_INF("Initializing MIPE BLE Service");

    /* Initialize Bluetooth */
    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        led_set_pattern(LED_ID_ERROR, LED_PATTERN_ERROR);
        return;
    }

    LOG_INF("Bluetooth initialized");

    /* Set TX power to maximum (+8 dBm) for better RSSI readings */
    #if defined(CONFIG_BT_CTLR_TX_PWR_PLUS_8)
        LOG_INF("TX Power set to maximum (+8 dBm) for optimal RSSI measurements");
    #else
        LOG_WRN("Maximum TX power not configured - using default");
    #endif

    /* Start advertising - using Host's proven method */
    err = bt_le_adv_start(&adv_param,
                         ad, ARRAY_SIZE(ad),
                         sd, ARRAY_SIZE(sd));
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)", err);
        led_set_pattern(LED_ID_ERROR, LED_PATTERN_ERROR);
        return;
    }

    LOG_INF("Advertising started - Device name: MIPE");
    /* Fix: Use LED1 for advertising, not LED3 */
    led_set_pattern(LED_ID_PAIRING, LED_PATTERN_ADVERTISING);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (err) {
        LOG_ERR("Failed to connect to %s (err %d)", addr, err);
        return;
    }

    LOG_INF("Connected to Host: %s", addr);
    
    current_conn = bt_conn_ref(conn);
    
    /* Update LEDs for connected state - Use LED1 for connection */
    led_set_pattern(LED_ID_PAIRING, LED_PATTERN_CONNECTED);    /* LED1 solid when connected */
    led_set_pattern(LED_ID_CONNECTION, LED_PATTERN_OFF);       /* LED2 off */
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Disconnected: %s (reason 0x%02x)", addr, reason);

    if (current_conn) {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }

    battery_notify_enabled = false;

    /* Update LEDs for disconnected state */
    led_set_pattern(LED_ID_PAIRING, LED_PATTERN_OFF);     /* LED1 off */
    led_set_pattern(LED_ID_CONNECTION, LED_PATTERN_OFF);  /* LED2 off */
    led_set_pattern(LED_ID_DATA, LED_PATTERN_OFF);        /* LED3 off */

    /* Stop advertising first */
    bt_le_adv_stop();
    
    /* Small delay for clean transition */
    k_msleep(100);

    /* Restart advertising */
    int err = bt_le_adv_start(&adv_param,
                              ad, ARRAY_SIZE(ad),
                              sd, ARRAY_SIZE(sd));
    if (err) {
        LOG_ERR("Failed to restart advertising (err %d)", err);
        led_set_pattern(LED_ID_ERROR, LED_PATTERN_ERROR);
    } else {
        LOG_INF("Advertising restarted");
        led_set_pattern(LED_ID_PAIRING, LED_PATTERN_ADVERTISING);
    }
}

static ssize_t read_battery(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                            void *buf, uint16_t len, uint16_t offset)
{
    battery_level = battery_monitor_get_level();
    
    LOG_DBG("Battery read: %u%%", battery_level);
    
    return bt_gatt_attr_read(conn, attr, buf, len, offset,
                            &battery_level, sizeof(battery_level));
}

static void battery_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    battery_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
    
    LOG_INF("Battery notifications %s", battery_notify_enabled ? "enabled" : "disabled");
}

/* Power-optimized listening mode after disconnection */
void ble_service_start_listening_mode(void)
{
    /* For now, just use normal advertising - can optimize later */
    LOG_INF("Entering listening mode");
    led_set_pattern(LED_ID_PAIRING, LED_PATTERN_SLOW_BLINK);
}

/* Send battery notification */
int ble_service_notify_battery(void)
{
    if (!current_conn || !battery_notify_enabled) {
        return -ENOTCONN;
    }
    
    battery_level = battery_monitor_get_level();
    
    int err = bt_gatt_notify(current_conn, &mipe_service.attrs[2], 
                            &battery_level, sizeof(battery_level));
    if (err) {
        LOG_ERR("Failed to send battery notification (err %d)", err);
    }
    
    return err;
}

/* Update function called from main loop */
void ble_service_update(void)
{
    static uint32_t last_battery_update = 0;
    uint32_t now = k_uptime_get_32();
    
    /* Update battery every 60 seconds */
    if (current_conn && (now - last_battery_update) > 60000) {
        ble_service_notify_battery();
        last_battery_update = now;
    }
}

/* Check connection status */
bool ble_service_is_connected(void)
{
    return (current_conn != NULL);
}
