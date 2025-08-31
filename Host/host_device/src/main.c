#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <stdio.h>
#include <string.h>
#include "ble_service.h"

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

/* Advertising parameters - standard intervals (100ms) */
static struct bt_le_adv_param adv_param = BT_LE_ADV_PARAM_INIT(
    BT_LE_ADV_OPT_CONN,
    BT_GAP_ADV_FAST_INT_MIN_2,  /* 100ms min */
    BT_GAP_ADV_FAST_INT_MAX_2,  /* 100ms max - consistent interval */
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
    
    // Notify BLE service of connection
    ble_service_set_app_conn(conn);
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
        
        // Notify BLE service of disconnection
        ble_service_set_app_conn(NULL);
        
        // Set flag to restart advertising in main loop
        // This avoids trying to restart advertising immediately in the callback
        advertising_active = false;
        
        LOG_INF("Advertising restart scheduled for main loop");
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

    // Initialize BLE service FIRST (before Bluetooth stack)
    ble_service_init();

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
    uint32_t adv_restart_counter = 0;
    
    while (1) {
        // Handle advertising restart if needed
        if (!app_connected && !advertising_active) {
            adv_restart_counter++;
            
            // Wait 3 seconds before attempting to restart advertising
            if (adv_restart_counter >= 3) {
                LOG_INF("Attempting to restart advertising...");
                
                // Simple approach: just try to start advertising
                int err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
                if (err == 0) {
                    advertising_active = true;
                    adv_restart_counter = 0;
                    LOG_INF("Advertising restarted successfully");
                } else {
                    LOG_WRN("Failed to restart advertising (err %d), will retry in 5 seconds", err);
                    // Reset counter to try again in 5 seconds
                    adv_restart_counter = 0;
                }
            }
        } else {
            // Reset counter when connected or advertising is active
            adv_restart_counter = 0;
        }
        
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

// ========================================
// CONTROL COMMAND HANDLERS
// ========================================

void handle_start_stream(void)
{
    LOG_INF("Start stream command received");
    // TODO: Implement stream start logic
}

void handle_stop_stream(void)
{
    LOG_INF("Stop stream command received");
    // TODO: Implement stream stop logic
}

void handle_get_status(void)
{
    LOG_INF("Get status command received");
    // TODO: Implement status reporting logic
}

void handle_mipe_sync(void)
{
    LOG_INF("Mipe sync command received");
    // TODO: Implement Mipe synchronization logic
}
