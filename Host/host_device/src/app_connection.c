#include "app_connection.h"
#include "ble_service.h"
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <string.h>

LOG_MODULE_REGISTER(app_connection, LOG_LEVEL_INF);

// ========================================
// GLOBAL VARIABLES
// ========================================

static struct bt_conn *app_conn = NULL;
static bool advertising_active = false;
static bool app_connected = false;

// ========================================
// ADVERTISING DATA
// ========================================

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, APP_DEVICE_NAME, strlen(APP_DEVICE_NAME)),
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_TMT1_SERVICE_VAL),
};

// ========================================
// ADVERTISING PARAMETERS
// ========================================

static struct bt_le_adv_param adv_param = BT_LE_ADV_PARAM_INIT(
    BT_LE_ADV_OPT_CONN,                    // Connectable advertising
    BT_GAP_ADV_FAST_INT_MIN_2,             // 100ms min interval
    BT_GAP_ADV_FAST_INT_MAX_2,             // 150ms max interval
    NULL                                    // No peer address
);

// ========================================
// CONNECTION CALLBACKS
// ========================================

static void app_connected_cb(struct bt_conn *conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    
    if (err) {
        LOG_ERR("App connection failed (err %u)", err);
        app_connected = false;
        return;
    }
    
    LOG_INF("App connected: %s", addr);
    app_conn = bt_conn_ref(conn);
    app_connected = true;
    advertising_active = false;
    
    // Set connection in BLE service
    ble_service_set_app_conn(conn);
    
    // Send log data to App
    ble_service_send_log_data("Host device ready - App connected");
    
    // Request connection parameter update for stability
    struct bt_le_conn_param param = {
        .interval_min = BT_GAP_INIT_CONN_INT_MIN,  /* 30ms */
        .interval_max = BT_GAP_INIT_CONN_INT_MAX,  /* 50ms */
        .latency = 0,
        .timeout = 400,  /* 4 seconds */
    };
    
    err = bt_conn_le_param_update(conn, &param);
    if (err) {
        LOG_WRN("Failed to request connection parameter update: %d", err);
    } else {
        LOG_INF("Connection parameter update requested");
    }
}

static void app_disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    
    LOG_INF("App disconnected: %s (reason %u)", addr, reason);
    
    if (app_conn == conn) {
        bt_conn_unref(app_conn);
        app_conn = NULL;
        app_connected = false;
        
        // Clear connection in BLE service
        ble_service_set_app_conn(NULL);
        
        // Automatically restart advertising
        LOG_INF("Restarting advertising for App discovery");
        int err = app_connection_start_advertising();
        if (err) {
            LOG_ERR("Failed to restart advertising: %d", err);
        }
    }
}

static struct bt_conn_cb app_conn_callbacks = {
    .connected = app_connected_cb,
    .disconnected = app_disconnected_cb,
};

// ========================================
// PUBLIC FUNCTIONS
// ========================================

int app_connection_init(void)
{
    LOG_INF("Initializing App connection");
    
    // Register connection callbacks
    bt_conn_cb_register(&app_conn_callbacks);
    
    LOG_INF("App connection initialized successfully");
    return 0;
}

int app_connection_start_advertising(void)
{
    if (advertising_active) {
        LOG_WRN("Advertising already active");
        return 0;
    }
    
    if (app_connected) {
        LOG_WRN("App already connected, no need to advertise");
        return 0;
    }
    
    LOG_INF("Starting advertising to App");
    
    // Start advertising
    int err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        LOG_ERR("Failed to start advertising: %d", err);
        return err;
    }
    
    advertising_active = true;
    LOG_INF("Advertising started - Device name: %s", APP_DEVICE_NAME);
    return 0;
}

int app_connection_stop_advertising(void)
{
    if (!advertising_active) {
        LOG_WRN("Advertising not active");
        return 0;
    }
    
    LOG_INF("Stopping advertising to App");
    
    int err = bt_le_adv_stop();
    if (err) {
        LOG_ERR("Failed to stop advertising: %d", err);
        return err;
    }
    
    advertising_active = false;
    LOG_INF("Advertising stopped");
    return 0;
}

bool app_connection_is_connected(void)
{
    return app_connected;
}

struct bt_conn *app_connection_get_conn(void)
{
    return app_conn;
}

int app_connection_disconnect(void)
{
    if (!app_connected || !app_conn) {
        LOG_WRN("Not connected to App");
        return -ENOTCONN;
    }
    
    LOG_INF("Disconnecting from App");
    
    int err = bt_conn_disconnect(app_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    if (err) {
        LOG_ERR("Failed to disconnect: %d", err);
        return err;
    }
    
    return 0;
}

bool app_connection_is_advertising(void)
{
    return advertising_active;
}
