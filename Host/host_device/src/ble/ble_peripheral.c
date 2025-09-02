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
#include "ble_central.h"

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
static uint8_t rssi_data[5] = {0};   /* RSSI data packet buffer - NOW 5 BYTES: host_battery(2) + mipe_battery(2) + rssi(1) */
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

/* Data transmission work - no timer needed for real-time */
static struct k_work tx_work;

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

    /* Send raw RSSI data only */
    err = ble_peripheral_send_rssi_data(rssi, 0);  /* No timestamp */
    if (err == 0) {
        packet_count++;
    }
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

    /* Initialize work (no timer needed for real-time) */
    k_work_init(&tx_work, tx_work_handler);

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
    
    /* No timer to stop in real-time mode */

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
        
        /* Start scanning for Mipe when streaming starts */
        {
            int scan_err = ble_central_start_scan();
            if (scan_err) {
                LOG_ERR("Failed to start scanning for Mipe: %d", scan_err);
                /* Don't send log notification here - it causes buffer conflicts */
            } else {
                LOG_INF("Started scanning for Mipe device");
                /* Don't send log notification here - it causes buffer conflicts */
            }
        }
        
        /* No timer - data will be sent in real-time as received */
        
        if (streaming_state_cb) {
            streaming_state_cb(true);
        }
        LOG_INF("Data streaming started - real-time mode");
        break;
        
    case CMD_STOP_STREAM:
        rssi_notify_enabled = false;
        streaming_active = false;
        
        /* Stop scanning for Mipe when streaming stops */
        {
            int scan_err = ble_central_stop_scan();
            if (scan_err) {
                LOG_ERR("Failed to stop scanning for Mipe: %d", scan_err);
            } else {
                LOG_INF("Stopped scanning for Mipe device");
                /* Don't send log notification here - let the scan stop cleanly */
            }
        }
        
        /* No timer to stop in real-time mode */
        
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
    int err;
    
    if (!current_conn || !log_notify_enabled) {
        return -ENOTCONN;
    }

    /* Check if we can send notification without blocking */
    err = bt_gatt_notify(current_conn, log_char_attr, log_str, strlen(log_str));
    if (err == -ENOMEM) {
        /* Buffer full, skip this notification */
        LOG_DBG("Log notification buffer full, skipping");
        return -EAGAIN;
    }
    
    return err;
}

/* Forward declaration for getting battery values */
extern uint16_t ble_central_get_mipe_battery_mv(void);
extern uint16_t get_host_battery_mv(void);

int ble_peripheral_send_rssi_data(int8_t rssi_value, uint32_t timestamp)
{
    struct bt_conn_info info;
    int err;
    uint16_t host_battery_mv;
    uint16_t mipe_battery_mv;
    
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
        /* Log this at INFO level to make it visible */
        static uint32_t last_warn = 0;
        uint32_t now = k_uptime_get_32();
        if (now - last_warn > 1000) {  /* Log once per second */
            LOG_INF("RSSI notifications not enabled - waiting for START_STREAM command");
            LOG_INF("Current state: streaming=%d, notify_enabled=%d", 
                    streaming_active, rssi_notify_enabled);
            last_warn = now;
        }
        return -EACCES;
    }

    /* Get battery values */
    host_battery_mv = get_host_battery_mv();
    mipe_battery_mv = ble_central_get_mipe_battery_mv();
    
    /* Log every bundle sent to the app */
    LOG_INF("Battery Bundle: Host=%u mV, Mipe=%u mV, RSSI=%d dBm", 
            host_battery_mv, mipe_battery_mv, rssi_value);

    /* Build RSSI data packet - NEW 5-byte format with battery data */
    /* Bytes 0-1: Host battery (uint16_t, little-endian) */
    /* Bytes 2-3: Mipe battery (uint16_t, little-endian) */
    /* Byte 4: RSSI value (int8_t) */
    rssi_data[0] = host_battery_mv & 0xFF;
    rssi_data[1] = (host_battery_mv >> 8) & 0xFF;
    rssi_data[2] = mipe_battery_mv & 0xFF;
    rssi_data[3] = (mipe_battery_mv >> 8) & 0xFF;
    rssi_data[4] = (uint8_t)rssi_value;
    
    LOG_DBG("Sending packet: Host[0x%02X%02X] Mipe[0x%02X%02X] RSSI[%d]",
            rssi_data[1], rssi_data[0], rssi_data[3], rssi_data[2], rssi_value);

    /* Send notification with error handling */
    err = bt_gatt_notify(current_conn, rssi_char_attr, 
                        rssi_data, 5);  /* Now sending 5 bytes instead of 4 */
    
    if (err == -ENOMEM) {
        /* Buffer full, skip this notification */
        LOG_DBG("RSSI notification buffer full, skipping");
        return -EAGAIN;
    } else if (err) {
        LOG_ERR("Failed to send RSSI notification (err %d)", err);
        return err;
    }
    
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
    static uint32_t last_update_time = 0;
    uint32_t current_time = k_uptime_get_32();
    int err;
    
    if (!current_conn || !mipe_status_notify_enabled) {
        return -ENOTCONN;
    }

    /* Rate limit status updates to max 1 per second */
    if ((current_time - last_update_time) < 1000) {
        LOG_DBG("Mipe status update rate limited, skipping");
        return -EAGAIN;
    }

    memcpy(&mipe_status, status, sizeof(mipe_status_t));

    /* Create a properly formatted buffer for Android app compatibility */
    uint8_t formatted_data[16] = {0};
    
    /* Byte 0: status_flags */
    formatted_data[0] = mipe_status.status_flags;
    
    /* Byte 1: rssi - keep as signed int8_t */
    formatted_data[1] = (uint8_t)(mipe_status.rssi & 0xFF);
    
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

    /* Try to send notification with error handling */
    err = bt_gatt_notify(current_conn, mipe_status_char_attr, formatted_data, sizeof(formatted_data));
    
    if (err == -ENOMEM) {
        /* Buffer full, skip this notification */
        LOG_DBG("Mipe status notification buffer full, skipping");
        return -EAGAIN;
    } else if (err) {
        LOG_DBG("Failed to send Mipe status notification (err %d)", err);
        return err;
    }
    
    /* Only log success if notification was actually sent */
    if (err == 0) {
        LOG_INF("Sending formatted Mipe status: flags=0x%02x, rssi=%d, batt=%.2fV", 
                formatted_data[0], (int8_t)formatted_data[1], (double)battery_voltage);
    }
    
    last_update_time = current_time;
    return 0;
}
