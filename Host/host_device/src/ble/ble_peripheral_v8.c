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

#include "ble_peripheral.h"

LOG_MODULE_REGISTER(ble_peripheral_v8, LOG_LEVEL_INF);

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

/* Control commands */
#define CMD_START_STREAM 0x01
#define CMD_STOP_STREAM  0x02
#define CMD_GET_STATUS   0x03

/* Connection state */
static struct bt_conn *current_conn = NULL;
static bool rssi_notify_enabled = false;
static bool streaming_active = false;
static uint32_t packet_count = 0;

/* Callbacks - Extended API for v8 */
static void (*app_connected_cb)(void) = NULL;
static void (*app_disconnected_cb)(void) = NULL;
static void (*streaming_state_cb)(bool active) = NULL;
static int (*get_rssi_data_cb)(int8_t *rssi, uint32_t *timestamp) = NULL;

/* Status data */
static uint8_t host_status[8] = {0}; /* Status response buffer */
static uint8_t rssi_data[4] = {0};   /* RSSI data packet buffer */

/* Store the RSSI characteristic attribute pointer */
static const struct bt_gatt_attr *rssi_char_attr = NULL;

/* Forward declarations */
static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static ssize_t control_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                            const void *buf, uint16_t len, uint16_t offset, uint8_t flags);
static ssize_t status_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                          void *buf, uint16_t len, uint16_t offset);
static void rssi_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);

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
                       void (*stream_cb)(bool), int (*rssi_cb)(int8_t*, uint32_t*))
{
    LOG_INF("Initializing BLE Peripheral v8 for Host");

    app_connected_cb = conn_cb;
    app_disconnected_cb = disconn_cb;
    streaming_state_cb = stream_cb;
    get_rssi_data_cb = rssi_cb;

    /* Find and store the RSSI characteristic attribute */
    rssi_char_attr = &tmt1_service.attrs[2];
    LOG_INF("RSSI characteristic attribute stored at index 2");

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
