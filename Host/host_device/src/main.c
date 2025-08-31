#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <stdio.h>
#include <string.h>

LOG_MODULE_REGISTER(host_main, LOG_LEVEL_INF);

// ========================================
// GLOBAL VARIABLES
// ========================================

static struct bt_conn *app_conn = NULL;
static bool app_connected = false;
static bool advertising_active = false;

// ========================================
// ADVERTISING DATA
// ========================================

/* Advertising data - just device name */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, "MIPE_HOST_A1B2", 14),
};

/* Advertising parameters - fast advertising (100ms intervals) */
static struct bt_le_adv_param adv_param = BT_LE_ADV_PARAM_INIT(
    BT_LE_ADV_OPT_CONN,
    BT_GAP_ADV_FAST_INT_MIN_2,  /* 100ms min */
    BT_GAP_ADV_FAST_INT_MAX_2,  /* 150ms max */
    NULL
);

// ========================================
// BLUETOOTH READY CALLBACK
// ========================================

static void bt_ready(int err)
{
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return;
    }

    LOG_INF("Bluetooth initialized");
    LOG_INF("BLE Peripheral mode ready");

    // Start advertising
    err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("Advertising failed to start: %d", err);
        return;
    }

    advertising_active = true;
    LOG_INF("Advertising started - Device name: MIPE_HOST_A1B2");
}

// ========================================
// CONNECTION CALLBACKS
// ========================================

static void connected(struct bt_conn *conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    
    if (err) {
        LOG_ERR("Connection failed to %s (err %u)", addr, err);
        app_connected = false;
        return;
    }
    
    LOG_INF("App connected: %s", addr);
    app_conn = bt_conn_ref(conn);
    app_connected = true;
    advertising_active = false;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    
    LOG_INF("App disconnected: %s (reason %u)", addr, reason);
    
    if (app_conn == conn) {
        bt_conn_unref(app_conn);
        app_conn = NULL;
        app_connected = false;
        
        // Small delay to ensure clean disconnection
        k_msleep(100);
        
        // Restart advertising with proper error handling
        int err = bt_le_adv_stop();
        if (err && err != -EALREADY) {
            LOG_WRN("Failed to stop advertising (err %d)", err);
        }
        
        k_msleep(100);
        
        err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
        if (err) {
            LOG_ERR("Failed to restart advertising (err %d)", err);
            // Try again after a longer delay
            k_msleep(1000);
            err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
            if (err) {
                LOG_ERR("Second attempt to restart advertising failed (err %d)", err);
            } else {
                advertising_active = true;
                LOG_INF("Advertising restarted (second attempt)");
            }
        } else {
            advertising_active = true;
            LOG_INF("Advertising restarted");
        }
    }
}

static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
};

// ========================================
// MAIN APPLICATION
// ========================================

int main(void)
{
    int err;

    LOG_INF("Starting Host Device - Basic BLE Peripheral");
    LOG_INF("Board: nRF54L15DK");
    LOG_INF("MCU: nRF54L15 (ARM Cortex-M33)");
    LOG_INF("Features: BLE Peripheral for MotoApp connection");

    // Register connection callbacks
    bt_conn_cb_register(&conn_callbacks);

    // Initialize Bluetooth
    err = bt_enable(bt_ready);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return -1;
    }

    LOG_INF("Host device initialization complete");
    LOG_INF("Starting main application loop...");

    // Main application loop
    uint32_t counter = 0;
    
    while (1) {
        // Periodic status logging
        if (counter % 100 == 0) {
            LOG_INF("System running - Counter: %u", counter);
            LOG_INF("App connection: %s", app_connected ? "Connected" : "Disconnected");
            LOG_INF("Advertising: %s", advertising_active ? "Active" : "Inactive");
        }
        
        counter++;
        k_msleep(1000);  // 1 second delay
    }

    return 0;
}
