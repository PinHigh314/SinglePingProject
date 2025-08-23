/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * TMT1 BLE Peripheral implementation for MotoApp communication
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

LOG_MODULE_REGISTER(ble_peripheral, LOG_LEVEL_INF);

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
static ble_connection_cb_t connection_callback = NULL;
static data_stream_cb_t stream_callback = NULL;

/* GATT notification parameters */
static bool rssi_notify_enabled = false;
static struct bt_gatt_indicate_params rssi_ind_params;

/* Status data */
static uint8_t host_status[8] = {0}; /* Status response buffer */
static uint8_t rssi_data[4] = {0};   /* RSSI data packet buffer */

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

/* Advertising parameters */
static struct bt_le_adv_param adv_param = BT_LE_ADV_PARAM_INIT(
    BT_LE_ADV_OPT_CONN,
    BT_GAP_ADV_FAST_INT_MIN_2,
    BT_GAP_ADV_FAST_INT_MAX_2,
    NULL);

int ble_peripheral_init(ble_connection_cb_t conn_cb, data_stream_cb_t stream_cb)
{
    int err;

    LOG_INF("Initializing BLE Peripheral for TMT1");

    connection_callback = conn_cb;
    stream_callback = stream_cb;

    /* Initialize Bluetooth */
    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return err;
    }

    LOG_INF("Bluetooth initialized");

    /* Start advertising with new API */
    err = bt_le_adv_start(&adv_param,
                         ad, ARRAY_SIZE(ad),
                         sd, ARRAY_SIZE(sd));
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
    if (connection_callback) {
        connection_callback(true);
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    struct bt_conn_info info;
    int err;
    int retry_count = 0;

    bt_conn_get_info(conn, &info);
    
    /* Only handle Peripheral role disconnections (from MotoApp) */
    if (info.role != BT_CONN_ROLE_PERIPHERAL) {
        return;
    }

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("MotoApp Disconnected: %s (reason 0x%02x)", addr, reason);

    /* Clear connection state BEFORE notification */
    if (current_conn == conn) {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }

    rssi_notify_enabled = false;

    /* Notify application of disconnection */
    if (connection_callback) {
        connection_callback(false);
    }

    /* Ensure advertising is fully stopped */
    err = bt_le_adv_stop();
    if (err && err != -EALREADY) {
        LOG_WRN("Failed to stop advertising: %d", err);
    }
    
    /* Wait for BLE stack to stabilize */
    k_msleep(250);

    /* Try to restart advertising with retries */
    while (retry_count < 5) {
        err = bt_le_adv_start(&adv_param,
                              ad, ARRAY_SIZE(ad),
                              sd, ARRAY_SIZE(sd));
        
        if (err == 0) {
            LOG_INF("Advertising restarted successfully (attempt %d)", retry_count + 1);
            break;
        } else if (err == -EALREADY) {
            LOG_WRN("Advertising already active, stopping and retrying");
            bt_le_adv_stop();
            k_msleep(100);
        } else {
            LOG_ERR("Failed to restart advertising (err %d, attempt %d)", err, retry_count + 1);
            k_msleep(200 * (retry_count + 1)); /* Progressive backoff */
        }
        retry_count++;
    }
    
    if (retry_count >= 5) {
        LOG_ERR("Failed to restart advertising after 5 attempts - BLE may need reset");
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
        /* Force enable notifications when starting stream */
        rssi_notify_enabled = true;
        LOG_INF("Force enabled RSSI notifications for streaming");
        
        if (stream_callback) {
            stream_callback(true);
        }
        LOG_INF("Data streaming started");
        break;
        
    case CMD_STOP_STREAM:
        if (stream_callback) {
            stream_callback(false);
        }
        LOG_INF("Data streaming stopped");
        break;
        
    case CMD_GET_STATUS:
        /* Status will be read via the status characteristic */
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
    uint32_t packets = ble_peripheral_get_packet_count();

    /* Build status response */
    status_response[0] = rssi_notify_enabled ? 1 : 0; /* Streaming status */
    sys_put_le24(uptime, &status_response[1]);        /* Uptime (24-bit ms) */
    sys_put_le32(packets, &status_response[4]);       /* Packet count */

    return bt_gatt_attr_read(conn, attr, buf, len, offset,
                            status_response, sizeof(status_response));
}

static void rssi_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    rssi_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
    
    LOG_INF("RSSI notifications %s", rssi_notify_enabled ? "enabled" : "disabled");
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

    /* Ensure we're in connected state */
    if (info.state != BT_CONN_STATE_CONNECTED) {
        LOG_WRN("Connection not in connected state: %d", info.state);
        return -ENOTCONN;
    }

    if (!rssi_notify_enabled) {
        LOG_DBG("Cannot send RSSI - notifications not enabled");
        return -EACCES;
    }

    /* Build RSSI data packet */
    rssi_data[0] = (uint8_t)rssi_value;
    sys_put_le24(timestamp & 0xFFFFFF, &rssi_data[1]);

    /* Send notification using correct attribute index */
    /* attrs[1] = RSSI characteristic value attribute */
    err = bt_gatt_notify(current_conn, &tmt1_service.attrs[1], 
                            rssi_data, sizeof(rssi_data));
    
    if (err == -ENOMEM) {
        LOG_WRN("BLE buffer full, dropping RSSI packet");
        return err;
    } else if (err == -ENOTCONN) {
        LOG_ERR("Connection lost during notification");
        /* Clear connection reference if it's gone */
        if (current_conn) {
            bt_conn_unref(current_conn);
            current_conn = NULL;
        }
        return err;
    } else if (err) {
        LOG_ERR("Failed to send RSSI notification (err %d)", err);
        return err;
    }

    LOG_DBG("RSSI notification sent successfully: %d dBm", rssi_value);
    return 0;
}

uint32_t ble_peripheral_get_packet_count(void)
{
    /* This will be tracked by main.c */
    extern uint32_t packet_count;
    return packet_count;
}

bool ble_peripheral_is_connected(void)
{
    return (current_conn != NULL);
}

bool ble_peripheral_is_streaming(void)
{
    return rssi_notify_enabled;
}
