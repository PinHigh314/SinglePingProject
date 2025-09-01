#include "ble_service.h"
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <string.h>

LOG_MODULE_REGISTER(ble_service, LOG_LEVEL_INF);

// ========================================
// EXTERNAL FUNCTION DECLARATIONS
// ========================================

// These functions are defined in main.c
extern void handle_start_stream(void);
extern void handle_stop_stream(void);
extern void handle_get_status(void);
extern void handle_mipe_sync(void);

// ========================================
// GLOBAL VARIABLES
// ========================================

static struct bt_conn *app_conn = NULL;
static bool app_connected = false;

// ========================================
// CONTROL COMMAND HANDLER
// ========================================

static ssize_t control_write(struct bt_conn *conn,
                            const struct bt_gatt_attr *attr,
                            const void *buf,
                            uint16_t len,
                            uint16_t offset,
                            uint8_t flags)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    
    LOG_INF("=== CONTROL WRITE RECEIVED ===");
    LOG_INF("From: %s", addr);
    LOG_INF("Length: %d bytes", len);
    LOG_INF("Offset: %d", offset);
    LOG_INF("Flags: 0x%02x", flags);
    
    if (len > 0) {
        uint8_t cmd = ((uint8_t *)buf)[0];
        LOG_INF("Command byte: 0x%02x", cmd);
        
        // Log full command data
        LOG_INF("Full command data:");
        for (int i = 0; i < len && i < 16; i++) {
            LOG_INF("  [%d]: 0x%02x", i, ((uint8_t *)buf)[i]);
        }
        
        // Handle control command
        ble_service_handle_control_command(buf, len);
        LOG_INF("Command handled successfully");
    } else {
        LOG_WRN("Empty command received");
    }
    
    LOG_INF("=============================");
    return len;
}

// ========================================
// CONTROL READ HANDLER
// ========================================

static ssize_t control_read(struct bt_conn *conn,
                           const struct bt_gatt_attr *attr,
                           void *buf,
                           uint16_t len,
                           uint16_t offset)
{
    LOG_INF("=== CONTROL READ REQUESTED ===");
    
    // Return a simple test value
    uint8_t test_data[] = {0xAA, 0xBB, 0xCC, 0xDD};
    
    if (len > sizeof(test_data)) {
        len = sizeof(test_data);
    }
    
    memcpy(buf, test_data, len);
    LOG_INF("Control read completed - returned %d bytes", len);
    LOG_INF("=============================");
    return len;
}

// ========================================
// STATUS READ HANDLER
// ========================================

static ssize_t status_read(struct bt_conn *conn,
                           const struct bt_gatt_attr *attr,
                           void *buf,
                           uint16_t len,
                           uint16_t offset)
{
    LOG_INF("Status read requested");
    
    // Return basic status for now
    uint8_t status[] = {0x01, 0x00, 0x00, 0x00}; // Status: Ready
    
    if (len > sizeof(status)) {
        len = sizeof(status);
    }
    
    memcpy(buf, status, len);
    return len;
}

// ========================================
// TMT1 SERVICE DEFINITION
// ========================================

// CCC write callback for RSSI data notifications
static void rssi_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    LOG_INF("=== RSSI CCC CHANGED ===");
    LOG_INF("Value: 0x%04x", value);
    LOG_INF("Notifications: %s", (value == BT_GATT_CCC_NOTIFY) ? "ENABLED" : "DISABLED");
    LOG_INF("========================");
}



BT_GATT_SERVICE_DEFINE(tmt1_service,
    BT_GATT_PRIMARY_SERVICE(&tmt1_service_uuid),
    
    BT_GATT_CHARACTERISTIC(&rssi_data_uuid.uuid,
                           BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE,
                           NULL, NULL, NULL),
    BT_GATT_CCC(rssi_ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    
    BT_GATT_CHARACTERISTIC(&control_uuid.uuid,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                           control_read, control_write, NULL),
    
    BT_GATT_CHARACTERISTIC(&status_uuid.uuid,
                           BT_GATT_CHRC_READ,
                           BT_GATT_PERM_READ,
                           status_read, NULL, NULL),
    
    BT_GATT_CHARACTERISTIC(&mipe_status_uuid.uuid,
                           BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE,
                           NULL, NULL, NULL),
    BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    
    BT_GATT_CHARACTERISTIC(&log_data_uuid.uuid,
                           BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE,
                           NULL, NULL, NULL),
    BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
);

// ========================================
// PUBLIC FUNCTIONS
// ========================================

int ble_service_init(void)
{
    LOG_INF("Initializing BLE service");
    
    // TMT1 service is automatically registered by BT_GATT_SERVICE_DEFINE
    LOG_INF("BLE service initialized successfully");
    return 0;
}

bool ble_service_is_app_connected(void)
{
    return app_connected;
}

int ble_service_send_rssi_data(int8_t rssi, uint32_t timestamp)
{
    if (!app_connected || !app_conn) {
        LOG_ERR("Cannot send RSSI data: not connected");
        return -ENOTCONN;
    }
    
    // Prepare RSSI data packet (1 byte RSSI + 3 bytes timestamp)
    uint8_t data[4];
    data[0] = (uint8_t)rssi;
    data[1] = (uint8_t)(timestamp & 0xFF);
    data[2] = (uint8_t)((timestamp >> 8) & 0xFF);
    data[3] = (uint8_t)((timestamp >> 16) & 0xFF);
    
    LOG_INF("=== SENDING RSSI DATA ===");
    LOG_INF("RSSI: %d dBm", rssi);
    LOG_INF("Timestamp: %u ms", timestamp);
    LOG_INF("Data bytes: [0x%02x, 0x%02x, 0x%02x, 0x%02x]", 
            data[0], data[1], data[2], data[3]);
    
    // Send notification using the service attribute
    int err = bt_gatt_notify(app_conn, &tmt1_service.attrs[1], data, sizeof(data));
    if (err) {
        LOG_ERR("Failed to send RSSI data: %d", err);
        LOG_ERR("Error details: %s", 
                err == -ENOTCONN ? "Not connected" :
                err == -EINVAL ? "Invalid parameters" :
                err == -ENOMEM ? "No memory" :
                err == -EIO ? "I/O error" : "Unknown error");
        return err;
    }
    
    LOG_INF("RSSI data sent successfully");
    LOG_INF("========================");
    return 0;
}

int ble_service_send_mipe_status(uint8_t connection_state, int8_t rssi,
                                const uint8_t *device_address, uint32_t connection_duration,
                                float battery_voltage)
{
    if (!app_connected || !app_conn) {
        return -ENOTCONN;
    }
    
    // Prepare Mipe status packet (16 bytes total)
    uint8_t data[16];
    data[0] = connection_state;
    data[1] = (uint8_t)rssi;
    
    // Copy device address (6 bytes)
    if (device_address) {
        memcpy(&data[2], device_address, 6);
    } else {
        memset(&data[2], 0, 6);
    }
    
    // Connection duration (4 bytes, little-endian)
    data[8] = (uint8_t)(connection_duration & 0xFF);
    data[9] = (uint8_t)((connection_duration >> 8) & 0xFF);
    data[10] = (uint8_t)((connection_duration >> 16) & 0xFF);
    data[11] = (uint8_t)((connection_duration >> 24) & 0xFF);
    
    // Battery voltage (4 bytes, float, little-endian)
    memcpy(&data[12], &battery_voltage, 4);
    
    // Send notification using the service attribute
    int err = bt_gatt_notify(app_conn, &tmt1_service.attrs[7], data, sizeof(data));
    if (err) {
        LOG_ERR("Failed to send Mipe status: %d", err);
        return err;
    }
    
    LOG_DBG("Mipe status sent: state=%d, rssi=%d, duration=%u, battery=%.2f",
             connection_state, rssi, connection_duration, (double)battery_voltage);
    return 0;
}

int ble_service_send_log_data(const char *log_string)
{
    if (!app_connected || !app_conn) {
        return -ENOTCONN;
    }
    
    if (!log_string) {
        return -EINVAL;
    }
    
    // Send notification using the service attribute
    int err = bt_gatt_notify(app_conn, &tmt1_service.attrs[9], log_string, strlen(log_string));
    if (err) {
        LOG_ERR("Failed to send log data: %d", err);
        return err;
    }
    
    LOG_DBG("Log data sent: %s", log_string);
    return 0;
}

int ble_service_handle_control_command(const uint8_t *data, uint16_t len)
{
    if (!data || len == 0) {
        return -EINVAL;
    }
    
    uint8_t cmd = data[0];
    
    switch (cmd) {
        case CMD_START_STREAM:
            LOG_INF("Executing START STREAM command");
            handle_start_stream();
            break;
            
        case CMD_STOP_STREAM:
            LOG_INF("Executing STOP STREAM command");
            handle_stop_stream();
            break;
            
        case CMD_GET_STATUS:
            LOG_INF("Executing GET STATUS command");
            handle_get_status();
            break;
            
        case CMD_MIPE_SYNC:
            LOG_INF("Executing MIPE SYNC command");
            handle_mipe_sync();
            break;
            
        default:
            LOG_WRN("Unknown command: 0x%02x", cmd);
            break;
    }
    
    return 0;
}

// ========================================
// CONNECTION MANAGEMENT
// ========================================

void ble_service_set_app_conn(struct bt_conn *conn)
{
    app_conn = conn;
    app_connected = (conn != NULL);
    
    if (app_connected) {
        LOG_INF("App connected");
    } else {
        LOG_INF("App disconnected");
    }
}
