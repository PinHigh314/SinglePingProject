/**
 * BLE Service for Host Device - Minimal Working Version
 * 
 * This service provides basic BLE central functionality:
 * - Scanning for BLE devices
 * - Connecting to devices
 * - Basic BLE communication
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ble_service, LOG_LEVEL_INF);

/* Connection state */
static struct bt_conn *app_conn = NULL;
static volatile bool is_connected = false;
static volatile bool scanning_active = false;

/* Work queue for periodic operations */
static struct k_work_delayable scanning_work;

/* Forward declarations */
static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static void scanning_work_handler(struct k_work *work);

/* Connection callbacks */
BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

/**
 * Connection established callback
 */
static void connected(struct bt_conn *conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    
    if (err) {
        LOG_ERR("Connection failed to %s (err %d)", addr, err);
        return;
    }
    
    LOG_INF("Connected to: %s", addr);
    
    /* Store connection reference */
    app_conn = bt_conn_ref(conn);
    is_connected = true;
    
    /* Stop scanning when connected */
    scanning_active = false;
    bt_le_scan_stop();
}

/**
 * Connection lost callback
 */
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Disconnected from: %s (reason 0x%02x)", addr, reason);
    
    /* Clean up connection reference */
    if (app_conn) {
        bt_conn_unref(app_conn);
        app_conn = NULL;
    }
    
    is_connected = false;
    
    /* Restart scanning after disconnection */
    k_work_schedule(&scanning_work, K_MSEC(1000));
}

/**
 * Scanning work handler - periodically scans for BLE devices
 */
static void scanning_work_handler(struct k_work *work)
{
    if (is_connected || scanning_active) {
        return;
    }
    
    LOG_INF("Starting BLE scan...");
    
    /* Start scanning for devices */
    int err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
    if (err) {
        LOG_ERR("Failed to start scanning: %d", err);
        /* Try again in 5 seconds */
        k_work_schedule(&scanning_work, K_MSEC(5000));
        return;
    }
    
    scanning_active = true;
    LOG_INF("BLE scanning started");
    
    /* Stop scanning after 10 seconds */
    k_work_schedule(&scanning_work, K_MSEC(10000));
}

/**
 * Initialize BLE service
 */
int ble_service_init(void)
{
    /* Initialize the scanning work handler */
    k_work_init_delayable(&scanning_work, scanning_work_handler);
    
    LOG_INF("BLE service initialized - starting scan in 2 seconds");
    
    /* Start scanning after a delay */
    k_work_schedule(&scanning_work, K_MSEC(2000));
    
    return 0;
}

/**
 * Get connection status
 */
bool ble_service_is_connected(void)
{
    return is_connected;
}

/**
 * Get current connection
 */
struct bt_conn *ble_service_get_connection(void)
{
    return app_conn;
}

/**
 * Disconnect from current device
 */
void ble_service_disconnect(void)
{
    if (app_conn) {
        bt_conn_disconnect(app_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    }
}

/**
 * Start scanning for BLE devices
 */
int ble_service_start_scan(void)
{
    if (is_connected) {
        return -EALREADY;
    }
    
    if (scanning_active) {
        return -EINPROGRESS;
    }
    
    k_work_schedule(&scanning_work, K_MSEC(100));
    return 0;
}

/**
 * Stop scanning for BLE devices
 */
int ble_service_stop_scan(void)
{
    if (!scanning_active) {
        return -EALREADY;
    }
    
    bt_le_scan_stop();
    scanning_active = false;
    return 0;
}
