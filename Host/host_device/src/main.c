#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/gap.h>
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
static const uint32_t RSSI_SEND_INTERVAL = 100; // Send RSSI every 100ms

// ========================================
// MIPE DETECTION AND SCANNING
// ========================================

// Mipe scanning state
static bool mipe_scanning_active = false;
static bool mipe_device_found = false;
static char mipe_device_addr[BT_ADDR_LE_STR_LEN] = {0};
static int8_t mipe_rssi_value = -100; // Default RSSI value
static bt_addr_le_t mipe_addr_le;
static uint32_t last_mipe_detection = 0; // Track when Mipe was last seen

// Mipe device information
static const char *MIPE_EXPECTED_NAME = "MIPE";
static const size_t MIPE_NAME_LENGTH = 4;

// Time-multiplexed scanning/advertising state
static bool scanning_mode = false;
static uint32_t last_mode_switch = 0;
static const uint32_t SCAN_INTERVAL = 5000;   // 5 seconds scanning (reduced for faster Mipe detection)
static const uint32_t ADVERTISE_INTERVAL = 3000; // 3 seconds advertising (reduced for faster switching)

// BLE scanning parameters
static struct bt_le_scan_param scan_param = {
    .type = BT_LE_SCAN_TYPE_PASSIVE,
    .options = BT_LE_SCAN_OPT_NONE,
    .interval = BT_GAP_SCAN_FAST_INTERVAL,
    .window = BT_GAP_SCAN_FAST_WINDOW,
};

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
// MIPE SCANNING AND DETECTION
// ========================================

/**
 * BLE scanning callback - detects Mipe devices and gets real RSSI
 */
static void scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
                   struct net_buf_simple *buf)
{
    char addr_str[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
    
    // Check if this is a Mipe device by looking for the expected name
    if (adv_type == BT_GAP_ADV_TYPE_ADV_IND ||
        adv_type == BT_GAP_ADV_TYPE_ADV_SCAN_IND) {
        
        // Parse advertising data to find device name
        bool name_found = false;
        char device_name[32] = {0};
        
        // Create a copy of the buffer to parse
        struct net_buf_simple temp_buf = *buf;
        
        while (temp_buf.len > 0) {
            if (temp_buf.len < 2) break;
            
            uint8_t len = net_buf_simple_pull_u8(&temp_buf);
            if (len == 0 || len > temp_buf.len) break;
            
            uint8_t type = net_buf_simple_pull_u8(&temp_buf);
            
            if (type == BT_DATA_NAME_COMPLETE || type == BT_DATA_NAME_SHORTENED) {
                if (len > 1) {
                    int name_len = MIN(len - 1, sizeof(device_name) - 1);
                    memcpy(device_name, temp_buf.data, name_len);
                    device_name[name_len] = '\0';
                    name_found = true;
                    break;
                }
            }
            
            // Skip the data
            if (len > 1) {
                net_buf_simple_pull(&temp_buf, len - 1);
            }
        }
        
        // Log all devices found during scanning for debugging
        if (name_found) {
            LOG_INF("SCAN: Found device '%s' at %s (RSSI: %d dBm)", 
                   device_name, addr_str, rssi);
        } else {
            LOG_INF("SCAN: Found unnamed device at %s (RSSI: %d dBm)", 
                   addr_str, rssi);
        }
        
        // Check if this is a Mipe device - ONLY use MIPE, no fallbacks!
        if (name_found && strncmp(device_name, MIPE_EXPECTED_NAME, MIPE_NAME_LENGTH) == 0) {
            LOG_INF("=== MIPE DEVICE DETECTED ===");
            LOG_INF("Address: %s", addr_str);
            LOG_INF("Name: %s", device_name);
            LOG_INF("RSSI: %d dBm", rssi);
            LOG_INF("==========================");
            
            // Store Mipe device information
            mipe_device_found = true;
            strcpy(mipe_device_addr, addr_str);
            mipe_rssi_value = rssi; // Real RSSI value!
            memcpy(&mipe_addr_le, addr, sizeof(bt_addr_le_t));
            last_mipe_detection = k_uptime_get(); // Update detection time
            
            // Don't stop scanning - let it continue to update RSSI
        } else if (name_found) {
            // Log other devices but don't store their RSSI
            LOG_INF("Found other device: %s at %s (RSSI: %d dBm) - IGNORING", 
                   device_name, addr_str, rssi);
        }
    }
}

/**
 * Switch between advertising and scanning modes
 */
static int switch_to_scanning_mode(void)
{
    int err;
    
    // Stop advertising first
    if (advertising_active) {
        bt_le_adv_stop();
        advertising_active = false;
        LOG_INF("Advertising stopped for scanning mode");
    }
    
    // Wait a moment for advertising to fully stop
    k_msleep(100);
    
    // Start BLE scanning
    err = bt_le_scan_start(&scan_param, scan_cb);
    if (err) {
        LOG_ERR("Failed to start scanning: %d", err);
        return err;
    }
    
    scanning_mode = true;
    mipe_scanning_active = true;
    LOG_INF("=== SWITCHED TO SCANNING MODE ===");
    LOG_INF("Looking for device named '%s'", MIPE_EXPECTED_NAME);
    LOG_INF("================================");
    return 0;
}

static int switch_to_advertising_mode(void)
{
    int err;
    
    // Stop scanning first
    if (mipe_scanning_active) {
        bt_le_scan_stop();
        mipe_scanning_active = false;
        LOG_INF("Scanning stopped for advertising mode");
    }
    
    // Wait a moment for scanning to fully stop
    k_msleep(100);
    
    // Start advertising
    err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("Advertising failed to start: %d", err);
        return err;
    }
    
    scanning_mode = false;
    advertising_active = true;
    LOG_INF("=== SWITCHED TO ADVERTISING MODE ===");
    LOG_INF("Device name: MIPE_HOST_A1B2");
    LOG_INF("================================");
    return 0;
}





/**
 * Check if Mipe device is available and update RSSI
 */
static void check_mipe_status(void)
{
    static uint32_t last_status_check = 0;
    static uint32_t last_rssi_update = 0;
    uint32_t current_time = k_uptime_get();
    
    // Check if Mipe device has been lost (not detected for 10 seconds)
    if (mipe_device_found && (current_time - last_mipe_detection >= 10000)) {
        LOG_INF("=== MIPE DEVICE LOST ===");
        LOG_INF("No Mipe device detected for 10 seconds");
        LOG_INF("Clearing Mipe device state");
        LOG_INF("==========================");
        
        // Clear Mipe device state
        mipe_device_found = false;
        mipe_rssi_value = -100;
        memset(mipe_device_addr, 0, sizeof(mipe_device_addr));
        memset(&mipe_addr_le, 0, sizeof(bt_addr_le_t));
    }
    
    // Check every 10 seconds
    if (current_time - last_status_check >= 10000) {
        if (mipe_device_found) {
            LOG_INF("Mipe device available for RSSI reading - Current RSSI: %d dBm", mipe_rssi_value);
        } else {
            LOG_INF("Mipe device not found - will scan during next scan cycle");
        }
        last_status_check = current_time;
    }
    
    // Update RSSI every 2 seconds if Mipe is found
    if (mipe_device_found && (current_time - last_rssi_update >= 2000)) {
        LOG_INF("RSSI update: %d dBm", mipe_rssi_value);
        last_rssi_update = current_time;
    }
}

// ========================================
// RSSI DATA GENERATION
// ========================================

static int8_t generate_rssi_value(void)
{
    if (mipe_device_found) {
        // Use real RSSI from Mipe device
        LOG_INF("Using real RSSI from Mipe device: %d dBm", mipe_rssi_value);
        return mipe_rssi_value;
    } else {
        // No Mipe device found - return invalid RSSI
        LOG_WRN("No Mipe device found - returning invalid RSSI (-100)");
        return -100; // Invalid RSSI value
    }
}

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
    
    // Log BLE service details for debugging
    LOG_INF("BLE service ready for App commands");
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
    
    // Initialize with advertising mode first
    LOG_INF("Initializing in advertising mode...");
    advertising_active = true;
    scanning_mode = false;
    
    LOG_INF("Starting main application loop...");

    // Main application loop
    uint32_t counter = 0;
    
    while (1) {
        // Handle mode switching between advertising and scanning
        uint32_t current_time = k_uptime_get();
        
        if (!app_connected) {
            // Only switch modes when not connected to App
            if (scanning_mode) {
                // In scanning mode - switch to advertising after scan interval
                if (current_time - last_mode_switch >= SCAN_INTERVAL) {
                    switch_to_advertising_mode();
                    last_mode_switch = current_time;
                }
            } else {
                // In advertising mode - switch to scanning after advertise interval
                if (current_time - last_mode_switch >= ADVERTISE_INTERVAL) {
                    switch_to_scanning_mode();
                    last_mode_switch = current_time;
                }
            }
        } else {
            // App is connected - stay in advertising mode
            if (scanning_mode) {
                switch_to_advertising_mode();
                last_mode_switch = current_time;
            }
        }
        
        // Periodic status logging
        if (counter % 100 == 0) {
            LOG_INF("System running - Counter: %u", counter);
            LOG_INF("App connection: %s", app_connected ? "Connected" : "Disconnected");
            LOG_INF("Mode: %s", scanning_mode ? "SCANNING" : "ADVERTISING");
            LOG_INF("Advertising: %s", advertising_active ? "Active" : "Inactive");
            LOG_INF("Scanning: %s", mipe_scanning_active ? "Active" : "Inactive");
            LOG_INF("Streaming: %s (Count: %u)", streaming_active ? "Active" : "Inactive", stream_counter);
            LOG_INF("Mipe device found: %s", mipe_device_found ? "YES" : "NO");
            if (mipe_device_found) {
                LOG_INF("Current Mipe RSSI: %d dBm", mipe_rssi_value);
            }
            
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
        
        // Force Mipe scanning when not connected to ensure we find the device
        if (!app_connected && !mipe_device_found && !scanning_mode) {
            LOG_INF("No Mipe device found - forcing scan mode");
            switch_to_scanning_mode();
            last_mode_switch = current_time;
        }
        
        // Check BLE service status periodically
        static uint32_t last_ble_check = 0;
        if (app_connected && (counter - last_ble_check >= 50)) {  // Every 50 seconds when connected
            LOG_INF("BLE service status: App connected, waiting for commands");
            last_ble_check = counter;
        }
        
        // Send RSSI data if streaming is active (regardless of App connection)
        if (streaming_active) {
            uint32_t current_time = k_uptime_get();
            if (current_time - last_rssi_send >= RSSI_SEND_INTERVAL) {
                // Generate RSSI value
                int8_t rssi = generate_rssi_value();
                
                // Only send if we have a valid RSSI (not -100)
                if (rssi > -100) {
                    if (app_connected) {
                        // Send RSSI data via BLE service to App
                        int err = ble_service_send_rssi_data(rssi, current_time);
                        if (err == 0) {
                            LOG_INF("RSSI data sent to App: %d dBm, stream count: %u", rssi, stream_counter);
                            stream_counter++;
                            last_rssi_send = current_time;
                        } else {
                            LOG_ERR("Failed to send RSSI data to App: %d", err);
                        }
                    } else {
                        // App not connected - just log the RSSI reading
                        LOG_INF("RSSI reading (no App): %d dBm, stream count: %u", rssi, stream_counter);
                        stream_counter++;
                        last_rssi_send = current_time;
                    }
                } else {
                    LOG_WRN("Skipping RSSI send - no valid Mipe RSSI available");
                }
            }
        } else if (app_connected && !streaming_active) {
            // Log when connected but not streaming
            if (counter % 50 == 0) {  // More frequent logging when connected
                LOG_INF("App connected, waiting for start stream command");
            }
        }
        
        counter++;
        k_msleep(100);  // 100ms delay to match RSSI_SEND_INTERVAL
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
