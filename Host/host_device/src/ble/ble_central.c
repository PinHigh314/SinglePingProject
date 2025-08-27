/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * BLE Central REAL VERSION - BEACON MODE
 * Scans for Mipe device advertising packets to read RSSI for distance measurement
 * No BLE connection is established - Mipe is treated as a beacon only
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "ble_central.h"

LOG_MODULE_REGISTER(ble_central_real, LOG_LEVEL_INF);

/* Target device name */
#define MIPE_DEVICE_NAME "MIPE"

/* Callback management */
static mipe_rssi_cb_t mipe_rssi_callback = NULL;
static bool scanning = false;
static struct bt_conn *mipe_connection = NULL;
static bool connected = false;
static bt_addr_le_t mipe_device_addr;
static bool mipe_device_found = false;

/* Context for the data parser */
struct ad_parse_ctx {
    const bt_addr_le_t *addr;
    int8_t rssi;
};

/* Data parsing callback */
static bool ad_parse_cb(struct bt_data *data, void *user_data)
{
    struct ad_parse_ctx *ctx = user_data;
    char name[32];
    uint8_t name_len;

    if (data->type != BT_DATA_NAME_COMPLETE && data->type != BT_DATA_NAME_SHORTENED) {
        return true; /* Continue parsing */
    }

    name_len = MIN(data->data_len, sizeof(name) - 1);
    memcpy(name, data->data, name_len);
    name[name_len] = '\0';

    if (strcmp(name, MIPE_DEVICE_NAME) == 0) {
        char addr_str[BT_ADDR_LE_STR_LEN];
        bt_addr_le_to_str(ctx->addr, addr_str, sizeof(addr_str));
        LOG_INF("Found Mipe beacon: %s (RSSI %d)", addr_str, ctx->rssi);
        
        /* Store device address for sync mode connection */
        memcpy(&mipe_device_addr, ctx->addr, sizeof(bt_addr_le_t));
        mipe_device_found = true;
        
        /* Forward RSSI to callback immediately */
        if (mipe_rssi_callback) {
            mipe_rssi_callback(ctx->rssi, k_uptime_get_32());
        }
        
        return false; /* Stop parsing */
    }

    return true; /* Continue parsing */
}

/* Scan callback */
static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                        struct net_buf_simple *ad)
{
    char addr_str[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
    LOG_INF("Device found: %s (RSSI %d), type %u", addr_str, rssi, type);

    /* We're only interested in connectable events */
    if (type != BT_GAP_ADV_TYPE_ADV_IND && 
        type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND &&
        type != BT_GAP_ADV_TYPE_SCAN_RSP) {
        // return; // Temporarily disable filter to see all packets
    }

    struct ad_parse_ctx ctx = {
        .addr = addr,
        .rssi = rssi,
    };

    bt_data_parse(ad, ad_parse_cb, &ctx);
}

/* Connection callback forward declarations */
static void connected_cb(struct bt_conn *conn, uint8_t err);
static void disconnected_cb(struct bt_conn *conn, uint8_t reason);

/* Connection callbacks */
static struct bt_conn_cb conn_callbacks = {
    .connected = connected_cb,
    .disconnected = disconnected_cb,
};

int ble_central_init(mipe_rssi_cb_t rssi_cb)
{
    if (!rssi_cb) {
        LOG_ERR("Invalid callback provided");
        return -EINVAL;
    }

    mipe_rssi_callback = rssi_cb;

    /* Register connection callbacks */
    bt_conn_cb_register(&conn_callbacks);

    LOG_INF("BLE Central BEACON MODE initialized");
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

bool ble_central_is_scanning(void)
{
    return scanning;
}

/* Connection callback functions */
static void connected_cb(struct bt_conn *conn, uint8_t err)
{
    const bt_addr_le_t *addr = bt_conn_get_dst(conn);
    char addr_str[BT_ADDR_LE_STR_LEN];
    
    if (addr) {
        bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
    } else {
        strcpy(addr_str, "unknown");
    }
    
    if (err) {
        LOG_ERR("Failed to connect to %s (err %d)", addr_str, err);
        connected = false;
        return;
    }
    
    LOG_INF("Connected to Mipe: %s", addr_str);
    mipe_connection = conn;
    connected = true;
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
    const bt_addr_le_t *addr = bt_conn_get_dst(conn);
    char addr_str[BT_ADDR_LE_STR_LEN];
    
    if (addr) {
        bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
    } else {
        strcpy(addr_str, "unknown");
    }
    
    LOG_INF("Disconnected from Mipe: %s (reason 0x%02x)", addr_str, reason);
    
    if (mipe_connection == conn) {
        mipe_connection = NULL;
    }
    connected = false;
}

int ble_central_connect_to_mipe(const bt_addr_le_t *addr)
{
    int err;
    
    if (connected) {
        LOG_WRN("Already connected to Mipe");
        return 0;
    }
    
    struct bt_conn *conn;
    err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &conn);
    if (err) {
        LOG_ERR("Failed to create connection (err %d)", err);
        return err;
    }
    
    LOG_INF("Connection attempt initiated");
    return 0;
}

int ble_central_disconnect(void)
{
    if (!connected || !mipe_connection) {
        return 0;
    }
    
    int err = bt_conn_disconnect(mipe_connection, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    if (err) {
        LOG_ERR("Failed to disconnect (err %d)", err);
        return err;
    }
    
    LOG_INF("Disconnection initiated");
    return 0;
}

bool ble_central_is_connected(void)
{
    return connected;
}

/**
 * @brief Get the stored Mipe device address for connection
 * @param addr Pointer to store the device address
 * @return true if Mipe device address is available, false otherwise
 */
bool ble_central_get_mipe_address(bt_addr_le_t *addr)
{
    if (!mipe_device_found) {
        return false;
    }
    
    memcpy(addr, &mipe_device_addr, sizeof(bt_addr_le_t));
    return true;
}

/**
 * @brief Clear the stored Mipe device address
 */
void ble_central_clear_mipe_address(void)
{
    mipe_device_found = false;
    memset(&mipe_device_addr, 0, sizeof(bt_addr_le_t));
}
