/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/hci.h>

#include "ble_manager.h"

LOG_MODULE_REGISTER(ble_manager, LOG_LEVEL_INF);

/* BLE connection and GATT handles */
static struct bt_conn *current_conn = NULL;
static uint16_t ping_request_handle = 0;
static uint16_t ping_response_handle = 0;

/* Callbacks */
static connection_status_cb_t conn_status_cb = NULL;
static ping_response_cb_t ping_response_cb = NULL;

/* Scanning state */
static bool scanning = false;

/* Forward declarations */
static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                        struct net_buf_simple *ad);
static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static uint8_t discover_func(struct bt_conn *conn,
                            const struct bt_gatt_attr *attr,
                            struct bt_gatt_discover_params *params);
static uint8_t notify_func(struct bt_conn *conn,
                          struct bt_gatt_subscribe_params *params,
                          const void *data, uint16_t length);

/* BLE connection callbacks */
BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

/* GATT discovery parameters */
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;

int ble_manager_init(connection_status_cb_t conn_cb)
{
    int err;

    LOG_INF("Initializing BLE manager");

    conn_status_cb = conn_cb;

    /* Enable Bluetooth */
    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return err;
    }

    LOG_INF("Bluetooth initialized");
    return 0;
}

int ble_manager_start_scan(void)
{
    int err;
    struct bt_le_scan_param scan_param = {
        .type = BT_LE_SCAN_TYPE_ACTIVE,
        .options = BT_LE_SCAN_OPT_NONE,
        .interval = BT_GAP_SCAN_FAST_INTERVAL,
        .window = BT_GAP_SCAN_FAST_WINDOW,
    };

    if (scanning) {
        LOG_WRN("Already scanning");
        return 0;
    }

    LOG_INF("Starting BLE scan");

    err = bt_le_scan_start(&scan_param, device_found);
    if (err) {
        LOG_ERR("Scanning failed to start (err %d)", err);
        return err;
    }

    scanning = true;
    LOG_INF("BLE scanning started");
    return 0;
}

int ble_manager_stop_scan(void)
{
    int err;

    if (!scanning) {
        LOG_WRN("Not scanning");
        return 0;
    }

    LOG_INF("Stopping BLE scan");

    err = bt_le_scan_stop();
    if (err) {
        LOG_ERR("Failed to stop scanning (err %d)", err);
        return err;
    }

    scanning = false;
    LOG_INF("BLE scanning stopped");
    return 0;
}

int ble_manager_send_ping_request(const uint8_t *data, uint16_t len)
{
    int err;

    if (!current_conn) {
        LOG_ERR("No BLE connection");
        return -ENOTCONN;
    }

    if (ping_request_handle == 0) {
        LOG_ERR("Ping request characteristic not found");
        return -ENOENT;
    }

    LOG_DBG("Sending ping request, len: %d", len);

    err = bt_gatt_write_without_response(current_conn, ping_request_handle,
                                        data, len, false);
    if (err) {
        LOG_ERR("Failed to send ping request (err %d)", err);
        return err;
    }

    return 0;
}

void ble_manager_set_ping_response_callback(ping_response_cb_t cb)
{
    ping_response_cb = cb;
}

struct bt_conn *ble_manager_get_connection(void)
{
    return current_conn;
}

bool ble_manager_is_connected(void)
{
    return (current_conn != NULL);
}

static bool parse_device_ad(struct bt_data *data, void *user_data)
{
    bt_addr_le_t *addr = user_data;
    
    if (data->type == BT_DATA_UUID128_ALL || data->type == BT_DATA_UUID128_SOME) {
        struct bt_uuid_128 *uuid128 = (struct bt_uuid_128 *)SINGLEPING_SERVICE_UUID;
        
        if (data->data_len >= 16) {
            if (memcmp(data->data, uuid128->val, 16) == 0) {
                char addr_str[BT_ADDR_LE_STR_LEN];
                bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
                LOG_INF("Found SinglePing device: %s", addr_str);
                
                /* Stop scanning and connect */
                ble_manager_stop_scan();
                
                /* Connect to the device */
                struct bt_conn *conn;
                int err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
                                           BT_LE_CONN_PARAM_DEFAULT, &conn);
                if (err) {
                    LOG_ERR("Failed to create connection (err %d)", err);
                    /* Restart scanning */
                    ble_manager_start_scan();
                } else {
                    bt_conn_unref(conn);
                }
                
                return false; /* Stop parsing */
            }
        }
    }
    
    return true; /* Continue parsing */
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                        struct net_buf_simple *ad)
{
    if (type != BT_GAP_ADV_TYPE_ADV_IND &&
        type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
        return;
    }

    /* Parse advertisement data looking for SinglePing service UUID */
    bt_data_parse(ad, parse_device_ad, (void *)addr);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (err) {
        LOG_ERR("Failed to connect to %s (%u)", addr, err);
        /* Restart scanning */
        ble_manager_start_scan();
        return;
    }

    LOG_INF("Connected: %s", addr);

    current_conn = bt_conn_ref(conn);
    scanning = false;

    /* Start service discovery */
    discover_params.uuid = SINGLEPING_SERVICE_UUID;
    discover_params.func = discover_func;
    discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
    discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
    discover_params.type = BT_GATT_DISCOVER_PRIMARY;

    err = bt_gatt_discover(current_conn, &discover_params);
    if (err) {
        LOG_ERR("Discover failed (err %d)", err);
    }
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

    ping_request_handle = 0;
    ping_response_handle = 0;

    /* Notify application */
    if (conn_status_cb) {
        conn_status_cb(false);
    }
}

static uint8_t discover_func(struct bt_conn *conn,
                            const struct bt_gatt_attr *attr,
                            struct bt_gatt_discover_params *params)
{
    int err;

    if (!attr) {
        LOG_INF("Discover complete");
        (void)memset(params, 0, sizeof(*params));

        /* Notify application of successful connection */
        if (conn_status_cb) {
            conn_status_cb(true);
        }

        return BT_GATT_ITER_STOP;
    }

    LOG_INF("[ATTRIBUTE] handle %u", attr->handle);

    if (!bt_uuid_cmp(discover_params.uuid, SINGLEPING_SERVICE_UUID)) {
        LOG_INF("SinglePing service found");
        discover_params.uuid = PING_REQUEST_CHAR_UUID;
        discover_params.start_handle = attr->handle + 1;
        discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

        err = bt_gatt_discover(conn, &discover_params);
        if (err) {
            LOG_ERR("Discover failed (err %d)", err);
        }
    } else if (!bt_uuid_cmp(discover_params.uuid, PING_REQUEST_CHAR_UUID)) {
        LOG_INF("Ping request characteristic found");
        ping_request_handle = bt_gatt_attr_value_handle(attr);
        
        discover_params.uuid = PING_RESPONSE_CHAR_UUID;
        discover_params.start_handle = attr->handle + 1;
        discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

        err = bt_gatt_discover(conn, &discover_params);
        if (err) {
            LOG_ERR("Discover failed (err %d)", err);
        }
    } else if (!bt_uuid_cmp(discover_params.uuid, PING_RESPONSE_CHAR_UUID)) {
        LOG_INF("Ping response characteristic found");
        ping_response_handle = bt_gatt_attr_value_handle(attr);

        /* Subscribe to ping response notifications */
        subscribe_params.notify = notify_func;
        subscribe_params.value = BT_GATT_CCC_NOTIFY;
        subscribe_params.value_handle = ping_response_handle;
        subscribe_params.ccc_handle = ping_response_handle + 1;
        subscribe_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

        err = bt_gatt_subscribe(conn, &subscribe_params);
        if (err && err != -EALREADY) {
            LOG_ERR("Subscribe failed (err %d)", err);
        } else {
            LOG_INF("Subscribed to ping response notifications");
        }
    }

    return BT_GATT_ITER_CONTINUE;
}

static uint8_t notify_func(struct bt_conn *conn,
                          struct bt_gatt_subscribe_params *params,
                          const void *data, uint16_t length)
{
    if (!data) {
        LOG_INF("Unsubscribed");
        params->value_handle = 0U;
        return BT_GATT_ITER_STOP;
    }

    LOG_DBG("Ping response received, len: %d", length);

    /* Forward to ping response callback */
    if (ping_response_cb) {
        ping_response_cb(data, length);
    }

    return BT_GATT_ITER_CONTINUE;
}
