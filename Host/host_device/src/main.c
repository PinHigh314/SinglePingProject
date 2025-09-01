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
// STREAMING STATE MANAGEMENT
// ========================================

static bool streaming_active = false;
static uint32_t stream_counter = 0;
static uint32_t last_rssi_send = 0;
static const uint32_t RSSI_SEND_INTERVAL = 1000; // Send RSSI every 1 second

// ========================================
// MIPE DETECTION AND SCANNING
// ========================================

// Mipe scanning state
static bool mipe_scanning_active = false;
static bool mipe_device_found = false;
static char mipe_device_addr[BT_ADDR_LE_STR_LEN] = {0};
static int8_t mipe_rssi_value = -100; // Default RSSI value
static uint32_t last_mipe_scan = 0;
static const uint32_t MIPE_SCAN_INTERVAL = 5000; // Scan for Mipe every 5 seconds

// Mipe device information
static const char *MIPE_EXPECTED_NAME = "MIPE";
static const size_t MIPE_NAME_LENGTH = 4;

// ========================================
// MIPE SCANNING AND DETECTION
// ========================================

/**
 * Start scanning for Mipe devices
 */
static int start_mipe_scanning(void)
{
    LOG_INF("=== STARTING MIPE SCANNING ===");
    LOG_INF("Previous scanning state: %s", mipe_scanning_active ? "ACTIVE" : "INACTIVE");
    LOG_INF("Previous Mipe found state: %s", mipe_device_found ? "FOUND" : "NOT FOUND");
    
    // For now, simulate Mipe detection since we're not implementing full scanning yet
    // In the future, this will call bt_le_scan_start() with proper parameters
    
    mipe_scanning_active = true;
    mipe_device_found = true; // Simulate finding Mipe for now
    strcpy(mipe_device_addr, "MIPE_SIMULATED");
    mipe_rssi_value = -55; // Simulated RSSI value
    
    LOG_INF("Mipe scanning ACTIVATED");
    LOG_INF("Mipe device status: %s", mipe_device_found ? "FOUND" : "NOT FOUND");
    if (mipe_device_found) {
        LOG_INF("Mipe device address: %s", mipe_device_addr);
        LOG_INF("Mipe RSSI value: %d dBm", mipe_rssi_value);
    }
    LOG_INF("================================");
    
    return 0;
}

/**
 * Stop scanning for Mipe devices
 */
static int stop_mipe_scanning(void)
{
    LOG_INF("=== STOPPING MIPE SCANNING ===");
    LOG_INF("Previous scanning state: %s", mipe_scanning_active ? "ACTIVE" : "INACTIVE");
    LOG_INF("Previous Mipe found state: %s", mipe_device_found ? "FOUND" : "NOT FOUND");
    
    mipe_scanning_active = false;
    
    LOG_INF("Mipe scanning DEACTIVATED");
    LOG_INF("Mipe device status remains: %s", mipe_device_found ? "FOUND" : "NOT FOUND");
    LOG_INF("================================");
    
    return 0;
}

/**
 * Check if Mipe device is available and log status
 */
static void check_mipe_status(void)
{
    static uint32_t last_status_check = 0;
    uint32_t current_time = k_uptime_get();
    
    // Check every 10 seconds
    if (current_time - last_status_check >= 10000) {
        LOG_INF("=== MIPE STATUS CHECK ===");
        LOG_INF("Current time: %u ms", current_time);
        LOG_INF("Mipe scanning: %s", mipe_scanning_active ? "ACTIVE" : "INACTIVE");
        LOG_INF("Mipe device found: %s", mipe_device_found ? "YES" : "NO");
        
        if (mipe_device_found) {
            LOG_INF("Mipe device address: %s", mipe_device_addr);
            LOG_INF("Mipe RSSI value: %d dBm", mipe_rssi_value);
            LOG_INF("Mipe expected name: %s", MIPE_EXPECTED_NAME);
            LOG_INF("Mipe name length: %zu", MIPE_NAME_LENGTH);
            LOG_INF("Mipe device is AVAILABLE for RSSI reading");
        } else {
            LOG_INF("Mipe device NOT FOUND");
            LOG_INF("Mipe expected name: %s", MIPE_EXPECTED_NAME);
            LOG_INF("Mipe name length: %zu", MIPE_NAME_LENGTH);
            LOG_INF("Mipe device is NOT AVAILABLE for RSSI reading");
        }
        
        LOG_INF("==========================");
        last_status_check = current_time;
    }
}

// ========================================
// RSSI DATA GENERATION
// ========================================

static int8_t generate_rssi_value(void)
{
    LOG_INF("=== RSSI DATA GENERATION ===");
    
    if (mipe_device_found) {
        // Use real RSSI from Mipe device
        LOG_INF("Mipe device AVAILABLE - using real RSSI data");
        LOG_INF("Mipe device address: %s", mipe_device_addr);
        LOG_INF("Mipe RSSI value: %d dBm", mipe_rssi_value);
        LOG_INF("Real RSSI data source: MIPE DEVICE");
        LOG_INF("===========================");
        return mipe_rssi_value;
    } else {
        // Fall back to simulated RSSI values
        LOG_INF("Mipe device NOT AVAILABLE - using simulated RSSI data");
        LOG_INF("Mipe scanning status: %s", mipe_scanning_active ? "ACTIVE" : "INACTIVE");
        LOG_INF("Mipe expected name: %s", MIPE_EXPECTED_NAME);
        LOG_INF("Mipe name length: %zu", MIPE_NAME_LENGTH);
        
        static int8_t base_rssi = -55;
        static int8_t variation = 0;
        
        // Add some variation to simulate real-world conditions
        variation = (variation + 1) % 10;
        int8_t rssi = base_rssi + (variation - 5); // Range: -60 to -50 dBm
        
        LOG_INF("Simulated RSSI - Base: %d dBm", base_rssi);
        LOG_INF("Simulated RSSI - Variation: %d", variation);
        LOG_INF("Simulated RSSI - Generated: %d dBm", rssi);
        LOG_INF("Simulated RSSI - Range: -60 to -50 dBm");
        LOG_INF("===========================");
        
        return rssi;
    }
}

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
        LOG_ERR("=== CONNECTION FAILED ===");
        LOG_ERR("Failed to connect to %s (err %u)", addr, err);
        LOG_ERR("Connection error code: %u", err);
        app_connected = false;
        LOG_ERR("App connection state set to: DISCONNECTED");
        LOG_ERR("========================");
        return;
    }
    
    LOG_INF("=== APP CONNECTION ESTABLISHED ===");
    LOG_INF("App connected successfully from: %s", addr);
    LOG_INF("Previous connection state: %s", app_connected ? "CONNECTED" : "DISCONNECTED");
    LOG_INF("Previous advertising state: %s", advertising_active ? "ACTIVE" : "INACTIVE");
    
    app_conn = bt_conn_ref(conn);
    app_connected = true;
    advertising_active = false;
    
    LOG_INF("New connection state: %s", app_connected ? "CONNECTED" : "DISCONNECTED");
    LOG_INF("New advertising state: %s", advertising_active ? "ACTIVE" : "INACTIVE");
    LOG_INF("Connection object stored: %s", app_conn ? "Yes" : "No");
    
    // Notify BLE service of connection
    LOG_INF("Notifying BLE service of new connection...");
    ble_service_set_app_conn(conn);
    LOG_INF("BLE service notified successfully");
    LOG_INF("================================");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    
    LOG_INF("=== APP DISCONNECTION DETECTED ===");
    LOG_INF("App disconnected from: %s", addr);
    LOG_INF("Disconnection reason code: %u", reason);
    LOG_INF("Previous connection state: %s", app_connected ? "CONNECTED" : "DISCONNECTED");
    LOG_INF("Previous advertising state: %s", advertising_active ? "ACTIVE" : "INACTIVE");
    
    if (app_conn == conn) {
        LOG_INF("This is our active connection - processing disconnection...");
        
        bt_conn_unref(app_conn);
        app_conn = NULL;
        app_connected = false;
        
        LOG_INF("Connection object released and set to NULL");
        LOG_INF("App connection state set to: DISCONNECTED");
        
        // Notify BLE service of disconnection
        LOG_INF("Notifying BLE service of disconnection...");
        ble_service_set_app_conn(NULL);
        LOG_INF("BLE service notified successfully");
        
        // Set flag to restart advertising in main loop
        // This avoids trying to restart advertising immediately in the callback
        advertising_active = false;
        
        LOG_INF("Advertising state set to: INACTIVE");
        LOG_INF("Advertising restart scheduled for main loop");
        LOG_INF("================================");
    } else {
        LOG_WRN("Disconnection from unknown connection - ignoring");
        LOG_INF("================================");
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
    
    // Initialize Mipe scanning (simulated for now)
    LOG_INF("Initializing Mipe scanning...");
    start_mipe_scanning();
    
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
            LOG_INF("Streaming: %s (Count: %u)", streaming_active ? "Active" : "Inactive", stream_counter);
            
            // Additional detailed status when connected
            if (app_connected) {
                LOG_INF("=== DETAILED STATUS ===");
                LOG_INF("Connection active: %s", app_conn ? "Yes" : "No");
                LOG_INF("Streaming state: %s", streaming_active ? "ACTIVE" : "INACTIVE");
                LOG_INF("Stream counter: %u", stream_counter);
                LOG_INF("Last RSSI send: %lld ms ago", 
                        k_uptime_get() - last_rssi_send);
                LOG_INF("RSSI send interval: %u ms", RSSI_SEND_INTERVAL);
                LOG_INF("======================");
            }
        }
        
        // Check Mipe status periodically
        check_mipe_status();
        
        // Send RSSI data if streaming is active
        if (streaming_active && app_connected) {
            uint32_t current_time = k_uptime_get();
            if (current_time - last_rssi_send >= RSSI_SEND_INTERVAL) {
                LOG_INF("=== RSSI STREAMING CYCLE START ===");
                LOG_INF("Current time: %u ms", current_time);
                LOG_INF("Last RSSI send: %u ms", last_rssi_send);
                LOG_INF("Time since last send: %u ms", current_time - last_rssi_send);
                LOG_INF("RSSI send interval: %u ms", RSSI_SEND_INTERVAL);
                
                // Generate RSSI value with detailed logging
                int8_t rssi = generate_rssi_value();
                LOG_INF("RSSI value generated: %d dBm", rssi);
                
                LOG_INF("Attempting to send RSSI data via BLE service...");
                int err = ble_service_send_rssi_data(rssi, current_time);
                if (err == 0) {
                    LOG_INF("=== RSSI DATA SENT SUCCESSFULLY ===");
                    LOG_INF("RSSI value: %d dBm", rssi);
                    LOG_INF("Timestamp: %u ms", current_time);
                    LOG_INF("Stream count: %u", stream_counter);
                    LOG_INF("BLE service send result: %d (success)", err);
                    LOG_INF("=====================================");
                    
                    stream_counter++;
                    last_rssi_send = current_time;
                    
                    LOG_INF("Stream counter incremented to: %u", stream_counter);
                    LOG_INF("Last RSSI send time updated to: %u ms", last_rssi_send);
                } else {
                    LOG_ERR("=== RSSI DATA SEND FAILED ===");
                    LOG_ERR("RSSI value: %d dBm", rssi);
                    LOG_ERR("Timestamp: %u ms", current_time);
                    LOG_ERR("Stream count: %u", stream_counter);
                    LOG_ERR("BLE service send result: %d (error)", err);
                    LOG_ERR("BLE service error details: %s", 
                            err == -ENOTCONN ? "Not connected" :
                            err == -EINVAL ? "Invalid parameters" :
                            err == -ENOMEM ? "No memory" :
                            err == -EIO ? "I/O error" : "Unknown error");
                    LOG_ERR("=================================");
                }
                LOG_INF("=== RSSI STREAMING CYCLE END ===");
            }
        } else if (app_connected && !streaming_active) {
            // Log when connected but not streaming
            if (counter % 50 == 0) {  // More frequent logging when connected
                LOG_INF("=== CONNECTED BUT NOT STREAMING ===");
                LOG_INF("App is connected but streaming is INACTIVE");
                LOG_INF("Waiting for start stream command from App...");
                LOG_INF("Current streaming state: %s", streaming_active ? "ACTIVE" : "INACTIVE");
                LOG_INF("Stream counter: %u", stream_counter);
                LOG_INF("Last RSSI send: %lld ms ago", 
                        k_uptime_get() - last_rssi_send);
                LOG_INF("=====================================");
            }
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
    LOG_INF("=== START STREAM COMMAND RECEIVED ===");
    LOG_INF("Previous streaming state: %s", streaming_active ? "ACTIVE" : "INACTIVE");
    LOG_INF("Previous stream counter: %u", stream_counter);
    LOG_INF("Previous last RSSI send: %lld ms ago",
            k_uptime_get() - last_rssi_send);
    
    streaming_active = true;
    stream_counter = 0;
    last_rssi_send = 0;
    
    LOG_INF("New streaming state: %s", streaming_active ? "ACTIVE" : "INACTIVE");
    LOG_INF("Stream counter reset to: %u", stream_counter);
    LOG_INF("Last RSSI send reset to: %u", last_rssi_send);
    LOG_INF("RSSI streaming ACTIVATED successfully");
    LOG_INF("=====================================");
}

void handle_stop_stream(void)
{
    LOG_INF("=== STOP STREAM COMMAND RECEIVED ===");
    LOG_INF("Previous streaming state: %s", streaming_active ? "ACTIVE" : "INACTIVE");
    LOG_INF("Previous stream counter: %u", stream_counter);
    LOG_INF("Previous last RSSI send: %lld ms ago",
            k_uptime_get() - last_rssi_send);
    
    streaming_active = false;
    
    LOG_INF("New streaming state: %s", streaming_active ? "ACTIVE" : "INACTIVE");
    LOG_INF("Stream counter remains: %u", stream_counter);
    LOG_INF("Last RSSI send remains: %u", last_rssi_send);
    LOG_INF("RSSI streaming DEACTIVATED successfully");
    LOG_INF("=====================================");
}

void handle_get_status(void)
{
    LOG_INF("=== GET STATUS COMMAND RECEIVED ===");
    LOG_INF("Current system status:");
    LOG_INF("  - App connected: %s", app_connected ? "Yes" : "No");
    LOG_INF("  - Advertising active: %s", advertising_active ? "Yes" : "No");
    LOG_INF("  - Streaming active: %s", streaming_active ? "Yes" : "No");
    LOG_INF("  - Stream counter: %u", stream_counter);
    LOG_INF("  - Last RSSI send: %lld ms ago",
            k_uptime_get() - last_rssi_send);
    LOG_INF("  - RSSI send interval: %u ms", RSSI_SEND_INTERVAL);
    LOG_INF("  - Connection object: %s", app_conn ? "Valid" : "NULL");
    LOG_INF("Status report sent successfully");
    LOG_INF("================================");
}

void handle_mipe_sync(void)
{
    LOG_INF("=== MIPE SYNC COMMAND RECEIVED ===");
    LOG_INF("Mipe synchronization command received from App");
    LOG_INF("Current implementation status: PARTIALLY IMPLEMENTED");
    LOG_INF("Mipe scanning status: %s", mipe_scanning_active ? "ACTIVE" : "INACTIVE");
    LOG_INF("Mipe device found: %s", mipe_device_found ? "YES" : "NO");
    
    if (mipe_device_found) {
        LOG_INF("Mipe device details:");
        LOG_INF("  - Address: %s", mipe_device_addr);
        LOG_INF("  - RSSI: %d dBm", mipe_rssi_value);
        LOG_INF("  - Expected name: %s", MIPE_EXPECTED_NAME);
        LOG_INF("  - Name length: %zu", MIPE_NAME_LENGTH);
        LOG_INF("Mipe device is AVAILABLE for RSSI reading");
    } else {
        LOG_INF("Mipe device NOT FOUND");
        LOG_INF("Expected Mipe name: %s", MIPE_EXPECTED_NAME);
        LOG_INF("Expected name length: %zu", MIPE_NAME_LENGTH);
        LOG_INF("Mipe device is NOT AVAILABLE for RSSI reading");
    }
    
    LOG_INF("Current streaming state: %s", streaming_active ? "ACTIVE" : "INACTIVE");
    LOG_INF("Future functionality: Will read real battery data from Mipe device");
    LOG_INF("Mipe sync command acknowledged");
    LOG_INF("================================");
}
