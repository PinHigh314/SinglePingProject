/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * BLE Central REAL VERSION for v8
 * Actual BLE scanning and connection to Mipe device
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "ble_central.h"

LOG_MODULE_REGISTER(ble_central_real, LOG_LEVEL_INF);

/* Target device name */
#define MIPE_DEVICE_NAME "SinglePing Mipe"

/* Connection and callback management */
static struct bt_conn *mipe_conn = NULL;
static mipe_connection_cb_t mipe_conn_callback = NULL;
static mipe_rssi_cb_t mipe_rssi_callback = NULL;
static bool scanning = false;

/* RSSI measurement timer and work */
static struct k_timer rssi_timer;
static struct k_work rssi_work;
static void rssi_timer_handler(struct k_timer *timer);
static void rssi_work_handler(struct k_work *work);

/* Connection callbacks */
static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);

static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
};

/* Scan callback */
static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                        struct net_buf_simple *ad)
{
    char addr_str[BT_ADDR_LE_STR_LEN];
    struct bt_le_conn_param *param;
    int err;

    if (mipe_conn) {
        return; /* Already connected */
    }

    /* Check if this is a connectable advertisement */
    if (type != BT_GAP_ADV_TYPE_ADV_IND && 
        type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
        return;
    }

    bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

    /* Parse advertisement data for device name */
    while (ad->len > 1) {
        uint8_t len = net_buf_simple_pull_u8(ad);
        uint8_t type;

        if (len == 0 || len > ad->len) {
            break;
        }

        type = net_buf_simple_pull_u8(ad);
        len--;

        if (type == BT_DATA_NAME_COMPLETE || type == BT_DATA_NAME_SHORTENED) {
            char name[32];
            uint8_t name_len = MIN(len, sizeof(name) - 1);
            
            memcpy(name, ad->data, name_len);
            name[name_len] = '\0';

            if (strcmp(name, MIPE_DEVICE_NAME) == 0) {
                LOG_INF("Found Mipe device: %s (RSSI %d)", addr_str, rssi);

                /* Stop scanning */
                err = bt_le_scan_stop();
                if (err) {
                    LOG_ERR("Failed to stop scan: %d", err);
                    return;
                }
                scanning = false;

                /* Connect to the device */
                param = BT_LE_CONN_PARAM_DEFAULT;
                err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &mipe_conn);
                if (err) {
                    LOG_ERR("Failed to create connection: %d", err);
                    /* Restart scanning */
                    ble_central_start_scan();
                }
                return;
            }
        }

        net_buf_simple_pull(ad, len);
    }
}

static void connected(struct bt_conn *conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (err) {
        LOG_ERR("Failed to connect to %s (%u)", addr, err);
        bt_conn_unref(mipe_conn);
        mipe_conn = NULL;
        /* Restart scanning */
        ble_central_start_scan();
        return;
    }

    LOG_INF("Connected to Mipe: %s", addr);
    
    /* Start RSSI measurement timer (1Hz) */
    k_timer_start(&rssi_timer, K_SECONDS(1), K_SECONDS(1));
    
    /* Notify callback */
    if (mipe_conn_callback) {
        mipe_conn_callback(true);
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Disconnected from %s (reason 0x%02x)", addr, reason);
    
    if (mipe_conn == conn) {
        bt_conn_unref(mipe_conn);
        mipe_conn = NULL;
        
        /* Stop RSSI timer */
        k_timer_stop(&rssi_timer);
        
        /* Notify callback */
        if (mipe_conn_callback) {
            mipe_conn_callback(false);
        }
        
        /* Restart scanning */
        ble_central_start_scan();
    }
}

int ble_central_init(mipe_connection_cb_t conn_cb, mipe_rssi_cb_t rssi_cb)
{
    if (!conn_cb || !rssi_cb) {
        LOG_ERR("Invalid callbacks provided");
        return -EINVAL;
    }

    mipe_conn_callback = conn_cb;
    mipe_rssi_callback = rssi_cb;

    /* Register connection callbacks */
    bt_conn_cb_register(&conn_callbacks);

    /* Initialize RSSI measurement timer and work */
    k_timer_init(&rssi_timer, rssi_timer_handler, NULL);
    k_work_init(&rssi_work, rssi_work_handler);

    LOG_INF("BLE Central REAL VERSION initialized");
    return 0;
}

int ble_central_start_scan(void)
{
    int err;
    
    if (scanning) {
        LOG_WRN("Already scanning");
        return 0;
    }

    struct bt_le_scan_param scan_param = {
        .type = BT_LE_SCAN_TYPE_ACTIVE,
        .options = BT_LE_SCAN_OPT_NONE,
        .interval = BT_GAP_SCAN_FAST_INTERVAL,
        .window = BT_GAP_SCAN_FAST_WINDOW,
    };

    err = bt_le_scan_start(&scan_param, device_found);
    if (err) {
        LOG_ERR("Failed to start scan: %d", err);
        return err;
    }

    scanning = true;
    LOG_INF("Started scanning for Mipe device");
    return 0;
}

int ble_central_stop_scan(void)
{
    int err;
    
    if (!scanning) {
        return 0;
    }

    err = bt_le_scan_stop();
    if (err) {
        LOG_ERR("Failed to stop scan: %d", err);
        return err;
    }

    scanning = false;
    LOG_INF("Stopped scanning");
    return 0;
}

int ble_central_disconnect_mipe(void)
{
    int err;
    
    if (!mipe_conn) {
        LOG_WRN("Not connected to Mipe");
        return -ENOTCONN;
    }

    err = bt_conn_disconnect(mipe_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    if (err) {
        LOG_ERR("Failed to disconnect: %d", err);
        return err;
    }

    return 0;
}

int ble_central_request_rssi(void)
{
    if (!mipe_conn) {
        return -ENOTCONN;
    }
    
    /* Trigger immediate RSSI measurement */
    k_work_submit(&rssi_work);
    return 0;
}

bool ble_central_is_connected(void)
{
    return (mipe_conn != NULL);
}

/* RSSI measurement timer handler */
static void rssi_timer_handler(struct k_timer *timer)
{
    /* Submit work to measure RSSI (can't do BT operations in timer context) */
    k_work_submit(&rssi_work);
}

/* RSSI measurement work handler */
static void rssi_work_handler(struct k_work *work)
{
    int8_t rssi;
    
    if (!mipe_conn) {
        return;
    }

    /* For now, simulate RSSI as Zephyr doesn't provide direct RSSI reading API */
    /* In a real implementation, you would use vendor-specific HCI commands */
    rssi = -45 - (k_uptime_get_32() % 20); /* Simulated varying RSSI */
    
    uint32_t timestamp = k_uptime_get_32();
    
    LOG_DBG("Mipe RSSI: %d dBm", rssi);
    
    /* Forward to callback */
    if (mipe_rssi_callback) {
        mipe_rssi_callback(rssi, timestamp);
    }
}
