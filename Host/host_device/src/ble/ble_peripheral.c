/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * BLE Peripheral implementation for v8 - Extended API
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>

#include "ble_peripheral.h"

LOG_MODULE_REGISTER(ble_peripheral_v8, LOG_LEVEL_INF);

/* LED3 definition for MIPE_SYNC command verification */
#define LED3_NODE DT_ALIAS(led3)
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);
static struct k_timer led3_flash_timer;

/* LED3 flash timer handler */
static void led3_flash_timer_handler(struct k_timer *timer)
{
    gpio_pin_set_dt(&led3, 0); /* Turn off LED3 */
}

/* Flash LED3 for MIPE_SYNC command verification */
static void flash_led3_sync_indicator(void)
{
    /* Check if LED3 device is ready */
    if (!gpio_is_ready_dt(&led3)) {
        LOG_WRN("LED3 not ready for sync indication");
        return;
    }
    
    /* Configure LED3 if not already configured */
    static bool led3_configured = false;
    if (!led3_configured) {
        int ret = gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);
        if (ret < 0) {
            LOG_WRN("Failed to configure LED3: %d", ret);
            return;
        }
        k_timer_init(&led3_flash_timer, led3_flash_timer_handler, NULL);
        led3_configured = true;
    }
    
    /* Flash LED3 for 1000ms */
    gpio_pin_set_dt(&led3, 1); /* Turn on LED3 */
    k_timer_start(&led3_flash_timer, K_MSEC(1000), K_NO_WAIT);
}

/* TMT1 Service UUID: 12345678-1234-5678-1234-56789abcdef0 */
#define TMT1_SERVICE_UUID BT_UUID_DECLARE_128( \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0))

/* RSSI Data Characteristic UUID: 12345678-1234-5678-1234-56789abcdef1 */
#define RSSI_DATA_CHAR_UUID BT_UUID_DECLARE_128( \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1))

/* Control Characteristic UUID: 12345678-1234-5678-1234-56789abcdef2 */
#define CONTROL_CHAR_UUID BT_UUID_DECLARE_128( \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef2))

/* Status Characteristic UUID: 12345678-1234-5678-1234-56789abcdef3 */
#define STATUS_CHAR_UUID BT_UUID_DECLARE_128( \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef3))

/* Mipe Status Characteristic UUID: 12345678-1234-5678-1234-56789abcdef4 */
#define MIPE_STATUS_CHAR_UUID BT_UUID_DECLARE_128( \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef4))

/* Log Data Characteristic UUID: 12345678-1234-5678-1234-56789abcdef5 */
#define LOG_DATA_CHAR_UUID BT_UUID_DECLARE_128( \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef5))

/* Control commands */
#define CMD_START_STREAM 0x01
#define CMD_STOP_STREAM  0x02
#define CMD_GET_STATUS   0x03
#define CMD_MIPE_SYNC    0x04

/* Connection state */
static struct bt_conn *current_conn = NULL;
static bool rssi_notify_enabled = false;
static bool mipe_status_notify_enabled = false;
static bool log_notify_enabled = false;
static bool streaming_active = false;
static uint32_t packet_count = 0;

/* Callbacks - Extended API for v8 */
static void (*app_connected_cb)(void) = NULL;
static void (*app_disconnected_cb)(void) = NULL;
static void (*streaming_state_cb)(bool active) = NULL;
static int (*get_rssi_data_cb)(int8_t *rssi, uint32_t *timestamp) = NULL;
static void (*mipe_sync_cb)(void) = NULL;

/* Status data */
static uint8_t host_status[8] = {0}; /* Status response buffer */
static uint8_t rssi_data[4] = {0};   /* RSSI data packet buffer */
static mipe_status_t mipe_status = {0}; /* Mipe status data */

/* Store the RSSI characteristic attribute pointer */
static const struct bt_gatt_attr *rssi_char_attr = NULL;
static const struct bt_gatt_attr *mipe_status_char_attr = NULL;
static const struct bt_gatt_attr *log_char_attr = NULL;

/* Forward declarations */
static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static ssize_t control_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                            const void *buf, uint16_t len, uint16_t offset, uint8_t flags);
static ssize_t status_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                          void *buf, uint16_t len, uint16_t offset);
static void rssi_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);
static void mipe_status_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);
static void log_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);

/* Connection callbacks */
BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

/* TMT1 Service Definition */
BT_GATT_SERVICE_DEFINE(tmt1_service,
    BT_GATT_PRIMARY_SERVICE(TMT1_SERVICE_UUID),
    
    /* RSSI Data Characteristic - Notify */
    BT_GATT_CHARACTERISTIC(RSSI_DATA_CHAR_UUID,
                          BT_GATT_CHRC_NOTIFY,
                          BT_GATT_PERM_NONE,
                          NULL, NULL, rssi_data),
    BT_GATT_CCC(rssi_ccc_cfg_changed,
               BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    
    /* Control Characteristic - Write */
    BT_GATT_CHARACTERISTIC(CONTROL_CHAR_UUID,
                          BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                          BT_GATT_PERM_WRITE,
                          NULL, control_write, NULL),
    
    /* Status Characteristic - Read */
    BT_GATT_CHARACTERISTIC(STATUS_CHAR_UUID,
                          BT_GATT_CHRC_READ,
                          BT_GATT_PERM_READ,
                          status_read, NULL, host_status),

    /* Mipe Status Characteristic - Read & Notify */
    BT_GATT_CHARACTERISTIC(MIPE_STATUS_CHAR_UUID,
                          BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                          BT_GATT_PERM_READ,
                          NULL, NULL, &mipe_status),
    BT_GATT_CCC(mipe_status_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

    /* Log Data Characteristic - Notify */
    BT_GATT_CHARACTERISTIC(LOG_DATA_CHAR_UUID,
                          BT_GATT_CHRC_NOTIFY,
                          BT_GATT_PERM_NONE,
                          NULL, NULL, NULL),
    BT_GATT_CCC(log_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

/* Advertising data */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL,
                  BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)),
};

/* Scan response data */
static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, "MIPE_HOST_A1B2", 14),
};

/* Data transmission work and timer */
static struct k_work tx_work;
static struct k_timer tx_timer;

static void tx_work_handler(struct k_work *work)
{
    int8_t rssi;
    uint32_t timestamp;
    int err;

    if (!streaming_active || !get_rssi_data_cb) {
        return;
    }

    /* Get RSSI data from callback */
    err = get_rssi_data_cb(&rssi, &timestamp);
    if (err) {
        return;
    }

    /* Send the data */
    err = ble_peripheral_send_rssi_data(rssi, timestamp);
    if (err == 0) {
        packet_count++;
    }
}

static void tx_timer_handler(struct k_timer *timer)
{
    k_work_submit(&tx_work);
}

/* Extended API initialization */
int ble_peripheral_init(void (*conn_cb)(void), void (*disconn_cb)(void),
                       void (*stream_cb)(bool), int (*rssi_cb)(int8_t*, uint32_t*),
                       void (*mipe_sync_cb)(void))
{
    LOG_INF("Initializing BLE Peripheral v8 for Host");

    app_connected_cb = conn_cb;
    app_disconnected_cb = disconn_cb;
    streaming_state_cb = stream_cb;
    get_rssi_data_cb = rssi_cb;
    mipe_sync_cb = mipe_sync_cb;

    /* Find and store the RSSI characteristic attribute */
    rssi_char_attr = &tmt1_service.attrs[2];
    mipe_status_char_attr = &tmt1_service.attrs[8];
    log_char_attr = &tmt1_service.attrs[11];
    LOG_INF("RSSI characteristic attribute stored at index 2");
    LOG_INF("Mipe status characteristic attribute stored at index 8");
    LOG_INF("Log characteristic attribute stored at index 11");

    /* Initialize work and timer */
    k_work_init(&tx_work, tx_work_handler);
    k_timer_init(&tx_timer, tx_timer_handler, NULL);

    LOG_INF("BLE Peripheral v8 initialized");
    return 0;
}

int ble_peripheral_start_advertising(void)
{
    int err;

    /* Advertising parameters */
    struct bt_le_adv_param adv_param = BT_LE_ADV_PARAM_INIT(
        BT_LE_ADV_OPT_CONN,
        BT_GAP_ADV_FAST_INT_MIN_2,
        BT_GAP_ADV_FAST_INT_MAX_2,
        NULL);

    err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return err;
    }

    LOG_INF("Advertising started - Device name: MIPE_HOST_A1B2");
    return 0;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    struct bt_conn_info info;

    bt_conn_get_info(conn, &info);
    
    /* Only handle Peripheral role connections (from MotoApp) */
    if (info.role != BT_CONN_ROLE_PERIPHERAL) {
        return;
    }

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (err) {
        LOG_ERR("Failed to connect to %s (err %d)", addr, err);
        return;
    }

    LOG_INF("MotoApp Connected: %s", addr);
    
    current_conn = bt_conn_ref(conn);
    
    /* Notify application of connection */
    if (app_connected_cb) {
        app_connected_cb();
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    struct bt_conn_info info;
    int err;

    bt_conn_get_info(conn, &info);
    
    /* Only handle Peripheral role disconnections (from MotoApp) */
    if (info.role != BT_CONN_ROLE_PERIPHERAL) {
        return;
    }

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("MotoApp Disconnected: %s (reason 0x%02x)", addr, reason);

    /* Clear connection state */
    if (current_conn == conn) {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }

    rssi_notify_enabled = false;
    mipe_status_notify_enabled = false;
    log_notify_enabled = false;
    streaming_active = false;
    
    /* Stop transmission timer */
    k_timer_stop(&tx_timer);

    /* Notify application of disconnection */
    if (app_disconnected_cb) {
        app_disconnected_cb();
    }

    /* Restart advertising */
    k_msleep(250);
    err = ble_peripheral_start_advertising();
    if (err) {
        LOG_ERR("Failed to restart advertising: %d", err);
    }
}

static ssize_t control_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                            const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    const uint8_t *data = buf;

    if (offset != 0 || len != 1) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    LOG_INF("Control command received: 0x%02x", data[0]);

    switch (data[0]) {
    case CMD_START_STREAM:
        rssi_notify_enabled = true;
        streaming_active = true;
        
        /* Start transmission timer at 10Hz */
        k_timer_start(&tx_timer, K_NO_WAIT, K_MSEC(100));
        
        if (streaming_state_cb) {
            streaming_state_cb(true);
        }
        LOG_INF("Data streaming started");
        break;
        
    case CMD_STOP_STREAM:
        rssi_notify_enabled = false;
        streaming_active = false;
        
        /* Stop transmission timer */
        k_timer_stop(&tx_timer);
        
        if (streaming_state_cb) {
            streaming_state_cb(false);
        }
        LOG_INF("Data streaming stopped");
        break;
        
    case CMD_GET_STATUS:
        LOG_INF("Status requested");
        break;
        
    case CMD_MIPE_SYNC:
        LOG_INF("MIPE_SYNC command received - initiating Mipe connection");
        /* Send log message to app */
        ble_peripheral_send_log_data("MIPE_SYNC: Starting Mipe connection");
        
        /* Flash LED3 to indicate sync command received */
        LOG_INF("MIPE_SYNC: Flashing LED3 for visual confirmation");
        ble_peripheral_send_log_data("MIPE_SYNC: LED3 flash - command received");
        flash_led3_sync_indicator(); /* Actually flash LED3 for 1000ms */
        
        /* Call the Mipe sync callback if registered */
        if (mipe_sync_cb) {
            mipe_sync_cb();
        } else {
            LOG_WRN("MIPE_SYNC command received but no callback registered");
            ble_peripheral_send_log_data("MIPE_SYNC: No callback registered");
        }
        break;
        
    default:
        LOG_WRN("Unknown control command: 0x%02x", data[0]);
        return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
    }

    return len;
}

static ssize_t status_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                          void *buf, uint16_t len, uint16_t offset)
{
    static uint8_t status_response[8];
    uint32_t uptime = k_uptime_get();

    /* Build status response */
    status_response[0] = streaming_active ? 1 : 0;    /* Streaming status */
    sys_put_le24(uptime, &status_response[1]);        /* Uptime (24-bit ms) */
    sys_put_le32(packet_count, &status_response[4]);  /* Packet count */

    return bt_gatt_attr_read(conn, attr, buf, len, offset,
                            status_response, sizeof(status_response));
}

static void rssi_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    bool notify_enabled = (value == BT_GATT_CCC_NOTIFY);
    
    LOG_INF("RSSI notifications %s via CCC", notify_enabled ? "enabled" : "disabled");
    
    /* Only update if explicitly disabled via CCC */
    if (!notify_enabled) {
        rssi_notify_enabled = false;
        streaming_active = false;
        k_timer_stop(&tx_timer);
        
        if (streaming_state_cb) {
            streaming_state_cb(false);
        }
    }
}

static void mipe_status_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    mipe_status_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
    LOG_INF("Mipe status notifications %s", mipe_status_notify_enabled ? "enabled" : "disabled");
}

static void log_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    log_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
    LOG_INF("Log notifications %s", log_notify_enabled ? "enabled" : "disabled");
}

int ble_peripheral_send_log_data(const char *log_str)
{
    if (!current_conn || !log_notify_enabled) {
        return -ENOTCONN;
    }

    return bt_gatt_notify(current_conn, log_char_attr, log_str, strlen(log_str));
}

int ble_peripheral_send_rssi_data(int8_t rssi_value, uint32_t timestamp)
{
    struct bt_conn_info info;
    int err;
    
    if (!current_conn) {
        LOG_WRN("Cannot send RSSI - no connection");
        return -ENOTCONN;
    }

    /* Verify connection is still valid */
    err = bt_conn_get_info(current_conn, &info);
    if (err) {
        LOG_ERR("Connection no longer valid: %d", err);
        return -ENOTCONN;
    }

    if (!rssi_notify_enabled) {
        LOG_DBG("Cannot send RSSI - notifications not enabled");
        return -EACCES;
    }

    /* Build RSSI data packet */
    rssi_data[0] = (uint8_t)rssi_value;
    sys_put_le24(timestamp & 0xFFFFFF, &rssi_data[1]);

    /* Send notification */
    err = bt_gatt_notify(current_conn, rssi_char_attr, 
                        rssi_data, sizeof(rssi_data));
    
    if (err) {
        LOG_ERR("Failed to send RSSI notification (err %d)", err);
        return err;
    }

    LOG_DBG("RSSI notification sent: %d dBm", rssi_value);
    return 0;
}

uint32_t ble_peripheral_get_packet_count(void)
{
    return packet_count;
}

bool ble_peripheral_is_connected(void)
{
    return (current_conn != NULL);
}

bool ble_peripheral_is_streaming(void)
{
    return streaming_active;
}

int ble_peripheral_update_mipe_status(const mipe_status_t *status)
{
    if (!current_conn || !mipe_status_notify_enabled) {
        return -ENOTCONN;
    }

    memcpy(&mipe_status, status, sizeof(mipe_status_t));

    /* Create a properly formatted buffer for Android app compatibility */
    uint8_t formatted_data[16] = {0};
    
    /* Byte 0: status_flags */
    formatted_data[0] = mipe_status.status_flags;
    
    /* Byte 1: rssi */
    formatted_data[1] = (uint8_t)mipe_status.rssi;
    
    /* Bytes 2-7: device address (placeholder) */
    /* For now, use a placeholder address since we don't have real device address */
    formatted_data[2] = 0xAA;
    formatted_data[3] = 0xBB;
    formatted_data[4] = 0xCC;
    formatted_data[5] = 0xDD;
    formatted_data[6] = 0xEE;
    formatted_data[7] = 0xFF;
    
    /* Bytes 8-11: connection_duration (UINT32_LE) */
    sys_put_le32(mipe_status.connection_duration, &formatted_data[8]);
    
    /* Bytes 12-15: battery_voltage (FLOAT) */
    float battery_voltage = mipe_status.battery_voltage;
    memcpy(&formatted_data[12], &battery_voltage, sizeof(float));

    LOG_INF("Sending formatted Mipe status: flags=0x%02x, rssi=%d, batt=%.2fV", 
            formatted_data[0], formatted_data[1], battery_voltage);

    return bt_gatt_notify(current_conn, mipe_status_char_attr, formatted_data, sizeof(formatted_data));
}
