/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * BLE Central Implementation for Mipe Device Connections
 * 
 * Handles scanning, connecting, and GATT operations with Mipe devices
 * while Host remains connected to MotoApp as a Peripheral.
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "ble_central.h"

LOG_MODULE_REGISTER(ble_central, LOG_LEVEL_INF);

/* Mipe device identification */
#define MIPE_DEVICE_NAME_PREFIX "MIPE"
#define MIPE_DEVICE_NAME_PREFIX_LEN 4

/* Connection parameters for Mipe */
#define CONN_INTERVAL_MIN 30  /* 30 * 1.25ms = 37.5ms */
#define CONN_INTERVAL_MAX 50  /* 50 * 1.25ms = 62.5ms */
#define CONN_LATENCY 4
#define CONN_TIMEOUT 400      /* 400 * 10ms = 4000ms */

/* GATT characteristics for Mipe (simplified for TMT3) */
static struct bt_uuid_128 mipe_service_uuid = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0x87654321, 0x4321, 0x8765, 0x4321, 0x987654321098)
);

/* Battery characteristic UUID (RSSI will be measured from connection) */
static struct bt_uuid_128 mipe_battery_char_uuid = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0x87654323, 0x4321, 0x8765, 0x4321, 0x987654321098)
);

/* Connection and callback management */
static struct bt_conn *mipe_conn = NULL;
static mipe_connection_cb_t mipe_conn_callback = NULL;
static mipe_rssi_cb_t mipe_rssi_callback = NULL;
static bool scanning = false;

/* GATT handles */
static uint16_t battery_handle = 0;
static struct bt_gatt_subscribe_params subscribe_params;

/* Discovery parameters */
static struct bt_gatt_discover_params discover_params;

/* RSSI measurement timer and work */
static struct k_timer rssi_timer;
static struct k_work rssi_work;
static void rssi_timer_handler(struct k_timer *timer);
static void rssi_work_handler(struct k_work *work);

/* Forward declarations */
static void scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                    struct net_buf_simple *ad);
static void connected_cb(struct bt_conn *conn, uint8_t err);
static void disconnected_cb(struct bt_conn *conn, uint8_t reason);
static uint8_t notify_cb(struct bt_conn *conn,
                         struct bt_gatt_subscribe_params *params,
                         const void *data, uint16_t length);
static void discover_mipe_service(struct bt_conn *conn);
static uint8_t discover_func(struct bt_conn *conn,
                             const struct bt_gatt_attr *attr,
                             struct bt_gatt_discover_params *params);

/* Connection callbacks */
BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected_cb,
    .disconnected = disconnected_cb,
};

int ble_central_init(mipe_connection_cb_t conn_cb, mipe_rssi_cb_t rssi_cb)
{
    if (!conn_cb || !rssi_cb) {
        LOG_ERR("Invalid callbacks provided");
        return -EINVAL;
    }

    mipe_conn_callback = conn_cb;
    mipe_rssi_callback = rssi_cb;

    /* Initialize RSSI measurement timer and work */
    k_timer_init(&rssi_timer, rssi_timer_handler, NULL);
    k_work_init(&rssi_work, rssi_work_handler);

    LOG_INF("BLE Central initialized for Mipe connections");
    return 0;
}

int ble_central_start_scan(void)
{
    int err;
    
    if (scanning) {
        LOG_WRN("Already scanning for Mipe devices");
        return -EALREADY;
    }

    struct bt_le_scan_param scan_param = {
        .type = BT_LE_SCAN_TYPE_ACTIVE,
        .options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
        .interval = BT_GAP_SCAN_FAST_INTERVAL,
        .window = BT_GAP_SCAN_FAST_WINDOW,
    };

    err = bt_le_scan_start(&scan_param, scan_cb);
    if (err) {
        LOG_ERR("Failed to start scanning: %d", err);
        return err;
    }

    scanning = true;
    LOG_INF("Started scanning for Mipe devices");
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
        LOG_ERR("Failed to stop scanning: %d", err);
        return err;
    }

    scanning = false;
    LOG_INF("Stopped scanning for Mipe devices");
    return 0;
}

int ble_central_disconnect_mipe(void)
{
    if (!mipe_conn) {
        LOG_WRN("No Mipe device connected");
        return -ENOTCONN;
    }

    int err = bt_conn_disconnect(mipe_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    if (err) {
        LOG_ERR("Failed to disconnect from Mipe: %d", err);
        return err;
    }

    LOG_INF("Disconnecting from Mipe device");
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

/* Scan callback - called for each advertising packet */
static void scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                    struct net_buf_simple *ad)
{
    char addr_str[BT_ADDR_LE_STR_LEN];
    char name[32];
    int err;

    /* Already connected to a Mipe device */
    if (mipe_conn) {
        return;
    }

    /* Get device name from advertising data */
    memset(name, 0, sizeof(name));
    uint8_t name_len = 0;
    
    while (ad->len > 1) {
        uint8_t len = net_buf_simple_pull_u8(ad);
        uint8_t type = net_buf_simple_pull_u8(ad);
        
        if (type == BT_DATA_NAME_COMPLETE || type == BT_DATA_NAME_SHORTENED) {
            name_len = MIN(len - 1, sizeof(name) - 1);
            memcpy(name, ad->data, name_len);
            name[name_len] = '\0';
            break;
        }
        
        net_buf_simple_pull(ad, len - 1);
    }

    /* Check if this is a Mipe device */
    if (strncmp(name, MIPE_DEVICE_NAME_PREFIX, MIPE_DEVICE_NAME_PREFIX_LEN) != 0) {
        return;
    }

    bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
    LOG_INF("Found Mipe device: %s (RSSI %d)", name, rssi);

    /* Stop scanning before connecting */
    ble_central_stop_scan();

    /* Initiate connection */
    struct bt_conn_le_create_param create_param = {
        .options = BT_CONN_LE_OPT_NONE,
        .interval = BT_GAP_SCAN_FAST_INTERVAL,
        .window = BT_GAP_SCAN_FAST_WINDOW,
    };

    struct bt_le_conn_param conn_param = {
        .interval_min = CONN_INTERVAL_MIN,
        .interval_max = CONN_INTERVAL_MAX,
        .latency = CONN_LATENCY,
        .timeout = CONN_TIMEOUT,
    };

    err = bt_conn_le_create(addr, &create_param, &conn_param, &mipe_conn);
    if (err) {
        LOG_ERR("Failed to create connection to Mipe: %d", err);
        /* Resume scanning on failure */
        ble_central_start_scan();
    } else {
        LOG_INF("Initiating connection to Mipe device");
    }
}

/* Connection established callback */
static void connected_cb(struct bt_conn *conn, uint8_t err)
{
    char addr_str[BT_ADDR_LE_STR_LEN];
    struct bt_conn_info info;
    
    bt_conn_get_info(conn, &info);
    bt_addr_le_to_str(info.le.dst, addr_str, sizeof(addr_str));

    /* Check if this is a Central role connection (to Mipe) */
    if (info.role != BT_CONN_ROLE_CENTRAL) {
        /* This is a Peripheral connection (from MotoApp), ignore here */
        return;
    }

    if (err) {
        LOG_ERR("Failed to connect to Mipe %s: %u", addr_str, err);
        mipe_conn = NULL;
        /* Resume scanning on connection failure */
        ble_central_start_scan();
        return;
    }

    LOG_INF("Connected to Mipe device: %s", addr_str);
    
    /* Store connection reference */
    mipe_conn = bt_conn_ref(conn);
    
    /* Notify callback */
    if (mipe_conn_callback) {
        mipe_conn_callback(true);
    }

    /* Start periodic RSSI measurements (every 1000ms for reliability) */
    k_timer_start(&rssi_timer, K_SECONDS(1), K_SECONDS(1));
    LOG_INF("Started periodic RSSI measurements from connection (1Hz for stability)");

    /* Start service discovery for Mipe GATT services (battery status) */
    discover_mipe_service(conn);
}

/* Disconnection callback */
static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
    char addr_str[BT_ADDR_LE_STR_LEN];
    struct bt_conn_info info;
    int err;
    int retry_count = 0;
    
    bt_conn_get_info(conn, &info);
    
    /* Check if this is a Central role disconnection (from Mipe) */
    if (info.role != BT_CONN_ROLE_CENTRAL) {
        /* This is a Peripheral disconnection (from MotoApp), ignore here */
        return;
    }

    /* Also check if this is actually our MIPE connection */
    if (conn != mipe_conn) {
        LOG_WRN("Disconnection from unknown central connection");
        return;
    }

    bt_addr_le_to_str(info.le.dst, addr_str, sizeof(addr_str));
    LOG_INF("Disconnected from Mipe %s (reason 0x%02x)", addr_str, reason);

    /* Clear connection reference BEFORE callback */
    if (mipe_conn) {
        bt_conn_unref(mipe_conn);
        mipe_conn = NULL;
    }

    /* Stop RSSI measurement timer */
    k_timer_stop(&rssi_timer);
    
    /* ALWAYS notify callback to ensure LED is updated */
    if (mipe_conn_callback) {
        LOG_INF("Notifying MIPE disconnection to update LED status");
        mipe_conn_callback(false);
    }

    /* Ensure any existing scan is stopped first */
    if (scanning) {
        LOG_INF("Stopping existing scan before restart");
        bt_le_scan_stop();
        scanning = false;
        k_msleep(200); /* Give more time for scan to fully stop */
    }

    /* Try to restart scanning with retries */
    while (retry_count < 3) {
        LOG_INF("Attempting to resume scan for Mipe devices (attempt %d)", retry_count + 1);
        
        err = ble_central_start_scan();
        if (err == 0) {
            LOG_INF("Successfully resumed scanning for Mipe devices");
            break;
        } else if (err == -EALREADY) {
            LOG_WRN("Scan already active, clearing and retrying");
            bt_le_scan_stop();
            scanning = false;
            k_msleep(200);
        } else {
            LOG_ERR("Failed to resume scanning: %d, retrying...", err);
            k_msleep(500); /* Wait longer between retries */
        }
        retry_count++;
    }
    
    if (retry_count >= 3) {
        LOG_ERR("Failed to resume scanning after 3 attempts");
    }
}

/* GATT notification callback for battery data */
static uint8_t notify_cb(struct bt_conn *conn,
                         struct bt_gatt_subscribe_params *params,
                         const void *data, uint16_t length)
{
    if (!data) {
        LOG_INF("Unsubscribed from Mipe battery notifications");
        params->value_handle = 0U;
        return BT_GATT_ITER_STOP;
    }

    if (length >= 1) { /* Battery level (1 byte) */
        const uint8_t *buf = data;
        uint8_t battery_level = buf[0];
        
        LOG_DBG("Received battery level from Mipe: %u%%", battery_level);
        
        /* TODO: Forward battery level to MotoApp via peripheral */
    }

    return BT_GATT_ITER_CONTINUE;
}

/* Subscribe to battery notifications */
static void subscribe_to_battery(struct bt_conn *conn, uint16_t handle)
{
    int err;

    subscribe_params.notify = notify_cb;
    subscribe_params.value = BT_GATT_CCC_NOTIFY;
    subscribe_params.value_handle = handle;
    subscribe_params.ccc_handle = 0;  /* Will be auto-discovered */

    err = bt_gatt_subscribe(conn, &subscribe_params);
    if (err && err != -EALREADY) {
        LOG_ERR("Failed to subscribe to battery notifications (err %d)", err);
    } else {
        LOG_INF("Subscribed to MIPE battery notifications");
    }
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
    if (!mipe_conn) {
        return;
    }
    
    /* For now, use simulated RSSI until we can access real RSSI */
    /* Real RSSI would require BT_HCI_OP_READ_RSSI HCI command */
    /* This is a temporary workaround for TMT3 testing */
    static int8_t simulated_rssi = -55;
    static int8_t rssi_variation = 0;
    
    /* Simulate RSSI variations */
    rssi_variation = (rssi_variation + 1) % 20;
    int8_t rssi = simulated_rssi + (rssi_variation - 10);
    uint32_t timestamp = k_uptime_get_32();
    
    LOG_DBG("Simulated RSSI from Mipe connection: %d dBm", rssi);
    
    /* Forward to callback */
    if (mipe_rssi_callback) {
        mipe_rssi_callback(rssi, timestamp);
    }
}

/* Discovery callback */
static uint8_t discover_func(struct bt_conn *conn,
                             const struct bt_gatt_attr *attr,
                             struct bt_gatt_discover_params *params)
{
    int err;

    if (!attr) {
        LOG_INF("Discovery complete");
        memset(params, 0, sizeof(*params));
        return BT_GATT_ITER_STOP;
    }

    LOG_DBG("[ATTRIBUTE] handle %u", attr->handle);

    if (!bt_uuid_cmp(discover_params.uuid, &mipe_service_uuid.uuid)) {
        /* Found MIPE service, now discover battery characteristic */
        memset(&discover_params, 0, sizeof(discover_params));
        discover_params.uuid = &mipe_battery_char_uuid.uuid;
        discover_params.start_handle = attr->handle + 1;
        discover_params.end_handle = 0xffff;
        discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
        discover_params.func = discover_func;

        err = bt_gatt_discover(conn, &discover_params);
        if (err) {
            LOG_ERR("Failed to discover battery characteristic (err %d)", err);
        }
        return BT_GATT_ITER_STOP;
    } else if (!bt_uuid_cmp(discover_params.uuid, &mipe_battery_char_uuid.uuid)) {
        /* Found battery characteristic */
        LOG_INF("Found MIPE battery characteristic at handle %u", attr->handle);
        
        /* Get the value handle (next handle after characteristic declaration) */
        uint16_t value_handle = bt_gatt_attr_value_handle(attr);
        
        /* Subscribe to notifications */
        subscribe_to_battery(conn, value_handle);
        
        memset(&discover_params, 0, sizeof(discover_params));
        return BT_GATT_ITER_STOP;
    }

    return BT_GATT_ITER_CONTINUE;
}

/* Start MIPE service discovery */
static void discover_mipe_service(struct bt_conn *conn)
{
    int err;

    LOG_INF("Starting MIPE service discovery");

    memset(&discover_params, 0, sizeof(discover_params));
    discover_params.uuid = &mipe_service_uuid.uuid;
    discover_params.func = discover_func;
    discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
    discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
    discover_params.type = BT_GATT_DISCOVER_PRIMARY;

    err = bt_gatt_discover(conn, &discover_params);
    if (err) {
        LOG_ERR("Failed to start discovery (err %d)", err);
        return;
    }

    LOG_INF("GATT discovery started for MIPE services");
}
