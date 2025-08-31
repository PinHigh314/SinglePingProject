#include "mipe_scanner.h"
#include "ble_service.h"
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <string.h>

LOG_MODULE_REGISTER(mipe_scanner, LOG_LEVEL_INF);

// ========================================
// GLOBAL VARIABLES
// ========================================

static struct bt_conn *mipe_conn = NULL;
static bool scanning_active = false;
static bool connected_to_mipe = false;
static int8_t last_rssi = 0;
static bt_addr_le_t mipe_address;
static uint32_t connection_start_time = 0;

// ========================================
// SCANNING CALLBACKS
// ========================================

static void scan_cb(const struct bt_le_scan_recv_info *info,
                    struct net_buf_simple *buf)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(info->addr, addr, sizeof(addr));
    
    // Check if this is a Mipe device
    if (info->rssi >= MIPE_RSSI_MIN && info->rssi <= MIPE_RSSI_MAX) {
        LOG_DBG("Mipe device found: %s, RSSI: %d dBm", addr, info->rssi);
        
        // Store RSSI data
        last_rssi = info->rssi;
        
        // Send RSSI data to App if connected
        if (ble_service_is_app_connected()) {
            uint32_t timestamp = k_uptime_get_32();
            ble_service_send_rssi_data(info->rssi, timestamp);
        }
        
        // Store Mipe address for potential connection
        memcpy(&mipe_address, info->addr, sizeof(bt_addr_le_t));
    }
}

static void scan_timeout_cb(void)
{
    LOG_INF("Scan timeout - no Mipe devices found");
    scanning_active = false;
}

static struct bt_le_scan_cb scan_callbacks = {
    .recv = scan_cb,
    .timeout = scan_timeout_cb,
};

// ========================================
// CONNECTION CALLBACKS
// ========================================

static void mipe_connected(struct bt_conn *conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    
    if (err) {
        LOG_ERR("Failed to connect to Mipe %s (err %u)", addr, err);
        connected_to_mipe = false;
        return;
    }
    
    LOG_INF("Connected to Mipe: %s", addr);
    mipe_conn = bt_conn_ref(conn);
    connected_to_mipe = true;
    connection_start_time = k_uptime_get_32();
    
    // Send connection status to App
    if (ble_service_is_app_connected()) {
        ble_service_send_mipe_status(2, last_rssi, (uint8_t *)addr, 0, 3.8f);
    }
}

static void mipe_disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    
    LOG_INF("Disconnected from Mipe %s (reason %u)", addr, reason);
    
    if (mipe_conn == conn) {
        bt_conn_unref(mipe_conn);
        mipe_conn = NULL;
        connected_to_mipe = false;
        
        // Calculate connection duration
        uint32_t connection_duration = k_uptime_get_32() - connection_start_time;
        
        // Send disconnection status to App
        if (ble_service_is_app_connected()) {
            ble_service_send_mipe_status(4, last_rssi, (uint8_t *)addr, connection_duration, 3.8f);
        }
    }
}

static struct bt_conn_cb mipe_conn_callbacks = {
    .connected = mipe_connected,
    .disconnected = mipe_disconnected,
};

// ========================================
// PUBLIC FUNCTIONS
// ========================================

int mipe_scanner_init(void)
{
    LOG_INF("Initializing Mipe scanner");
    
    // Register scan callbacks
    int err = bt_le_scan_cb_register(&scan_callbacks);
    if (err) {
        LOG_ERR("Failed to register scan callbacks: %d", err);
        return err;
    }
    
    // Register connection callbacks
    bt_conn_cb_register(&mipe_conn_callbacks);
    
    LOG_INF("Mipe scanner initialized successfully");
    return 0;
}

int mipe_scanner_start(void)
{
    if (scanning_active) {
        LOG_WRN("Scanner already active");
        return 0;
    }
    
    LOG_INF("Starting Mipe scanner");
    
    // Configure scan parameters
    struct bt_le_scan_param scan_param = BT_LE_SCAN_PARAM_INIT(
        BT_LE_SCAN_TYPE_PASSIVE,           // Passive scanning
        BT_LE_SCAN_OPT_NONE,               // No special options
        BT_GAP_SCAN_FAST_INTERVAL,         // Fast scanning
        BT_GAP_SCAN_FAST_WINDOW            // Fast window
    );
    
    // Start scanning
    int err = bt_le_scan_start(&scan_param, NULL);
    if (err) {
        LOG_ERR("Failed to start scanning: %d", err);
        return err;
    }
    
    scanning_active = true;
    LOG_INF("Mipe scanner started");
    return 0;
}

int mipe_scanner_stop(void)
{
    if (!scanning_active) {
        LOG_WRN("Scanner not active");
        return 0;
    }
    
    LOG_INF("Stopping Mipe scanner");
    
    int err = bt_le_scan_stop();
    if (err) {
        LOG_ERR("Failed to stop scanning: %d", err);
        return err;
    }
    
    scanning_active = false;
    LOG_INF("Mipe scanner stopped");
    return 0;
}

bool mipe_scanner_is_active(void)
{
    return scanning_active;
}

int mipe_scanner_connect_to_mipe(const bt_addr_le_t *addr)
{
    if (!addr) {
        return -EINVAL;
    }
    
    if (connected_to_mipe) {
        LOG_WRN("Already connected to Mipe");
        return -EALREADY;
    }
    
    char addr_str[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
    
    LOG_INF("Connecting to Mipe: %s", addr_str);
    
    // Configure connection parameters
    struct bt_le_conn_param conn_param = BT_LE_CONN_PARAM_INIT(
        BT_GAP_INIT_CONN_INT_MIN,          // 30ms min interval
        BT_GAP_INIT_CONN_INT_MAX,          // 50ms max interval
        0,                                  // No latency
        BT_GAP_INIT_CONN_TIMEOUT           // 4 second timeout
    );
    
    // Create connection
    int err = bt_conn_le_create(addr, &conn_param, &mipe_conn);
    if (err) {
        LOG_ERR("Failed to create connection: %d", err);
        return err;
    }
    
    // Send scanning status to App
    if (ble_service_is_app_connected()) {
        ble_service_send_mipe_status(1, last_rssi, (uint8_t *)addr, 0, 3.8f);
    }
    
    return 0;
}

int mipe_scanner_disconnect_from_mipe(void)
{
    if (!connected_to_mipe || !mipe_conn) {
        LOG_WRN("Not connected to Mipe");
        return -ENOTCONN;
    }
    
    LOG_INF("Disconnecting from Mipe");
    
    int err = bt_conn_disconnect(mipe_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    if (err) {
        LOG_ERR("Failed to disconnect: %d", err);
        return err;
    }
    
    return 0;
}

bool mipe_scanner_is_connected_to_mipe(void)
{
    return connected_to_mipe;
}

int mipe_scanner_read_battery(float *battery_voltage)
{
    if (!connected_to_mipe || !mipe_conn) {
        return -ENOTCONN;
    }
    
    if (!battery_voltage) {
        return -EINVAL;
    }
    
    // For now, return fake 3.8V battery reading
    *battery_voltage = 3.8f;
    
    LOG_INF("Battery reading: %.2fV (fake)", *battery_voltage);
    return 0;
}

int8_t mipe_scanner_get_last_rssi(void)
{
    return last_rssi;
}

int mipe_scanner_get_mipe_address(bt_addr_le_t *addr)
{
    if (!addr) {
        return -EINVAL;
    }
    
    // Return the last known address from scanning, even if not connected
    memcpy(addr, &mipe_address, sizeof(bt_addr_le_t));
    return 0;
}
