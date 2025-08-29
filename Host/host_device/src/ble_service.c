#include "ble_service.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <string.h>

LOG_MODULE_REGISTER(ble_service, LOG_LEVEL_INF);

static struct bt_conn *app_conn = NULL;
static bool streaming_active = false;
static struct k_work_delayable streaming_work;

/* Measurement Data Characteristic */
static uint8_t measurement_data[20];

/* Control Command Characteristic */
static uint8_t control_command[1];

/* System Status Characteristic */
static uint8_t system_status[4];

/* Mipe Status Characteristic */
static uint8_t mipe_status[16];  /* 12 bytes basic + 4 bytes battery voltage */

/* Log Data Characteristic */
static uint8_t log_data[128];

/* Control Commands */
#define CMD_START_STREAM    0x01
#define CMD_STOP_STREAM     0x02
#define CMD_GET_STATUS      0x03
#define CMD_MIPE_SYNC       0x04

/* Simulated RSSI range */
#define RSSI_MIN            -75
#define RSSI_MAX            -40
#define RSSI_VARIATION      2

static int8_t current_rssi = -55;  /* Starting RSSI value */

static void on_measurement_data_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    LOG_INF("Measurement Data CCCD updated: %s", (value == BT_GATT_CCC_NOTIFY) ? "enabled" : "disabled");
}

/* Forward declarations */
static void streaming_work_handler(struct k_work *work);
static int send_mipe_status_notification(const uint8_t *data, uint16_t len);

/* Generate simulated RSSI with realistic variations */
static int8_t generate_simulated_rssi(void)
{
    /* Add random variation to current RSSI */
    int variation = (sys_rand32_get() % (2 * RSSI_VARIATION + 1)) - RSSI_VARIATION;
    current_rssi += variation;
    
    /* Clamp to valid range */
    if (current_rssi < RSSI_MIN) {
        current_rssi = RSSI_MIN;
    } else if (current_rssi > RSSI_MAX) {
        current_rssi = RSSI_MAX;
    }
    
    return current_rssi;
}

/* Streaming work handler - sends simulated data every 100ms */
static void streaming_work_handler(struct k_work *work)
{
    if (!streaming_active || !app_conn) {
        return;
    }
    
    /* Generate simulated measurement packet */
    uint8_t packet[20] = {0};
    
    /* Packet format:
     * Byte 0-1: Header (0xAA55)
     * Byte 2: Packet Type (0x01 = RSSI Data)
     * Byte 3: RSSI Value (signed)
     * Byte 4-7: Timestamp (32-bit, milliseconds)
     * Byte 8: Link Quality (simulated)
     * Byte 9: TX Power Level (-20 dBm)
     * Byte 10: Channel Number (37)
     * Byte 11: Connection Status (0x01 = connected)
     * Byte 12: Mipe Battery Level (simulated 85%)
     * Byte 13-15: Reserved
     * Byte 16-17: Sequence number
     * Byte 18-19: CRC (simplified)
     */
    
    static uint16_t sequence = 0;
    uint32_t timestamp = k_uptime_get_32();
    
    packet[0] = 0xAA;
    packet[1] = 0x55;
    packet[2] = 0x01;  /* RSSI Data packet */
    packet[3] = generate_simulated_rssi();
    packet[4] = (timestamp >> 0) & 0xFF;
    packet[5] = (timestamp >> 8) & 0xFF;
    packet[6] = (timestamp >> 16) & 0xFF;
    packet[7] = (timestamp >> 24) & 0xFF;
    packet[8] = 200;   /* Link quality (0-255) */
    packet[9] = -20;   /* TX Power */
    packet[10] = 37;   /* BLE channel */
    packet[11] = 0x01; /* Connected status */
    packet[12] = 85;   /* Battery level */
    packet[16] = (sequence >> 0) & 0xFF;
    packet[17] = (sequence >> 8) & 0xFF;
    
    /* Simple CRC */
    uint16_t crc = 0;
    for (int i = 0; i < 18; i++) {
        crc += packet[i];
    }
    packet[18] = (crc >> 0) & 0xFF;
    packet[19] = (crc >> 8) & 0xFF;
    
    sequence++;
    
    /* Send the packet */
    int err = ble_service_send_measurement_data(packet, sizeof(packet));
    if (err) {
        LOG_WRN("Failed to send measurement data: %d", err);
    }
    
    /* Schedule next transmission (100ms) */
    k_work_schedule(&streaming_work, K_MSEC(100));
}

static ssize_t on_control_command_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                       const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    if (len > 0) {
        uint8_t cmd = ((uint8_t *)buf)[0];
        LOG_INF("Control command received: 0x%02x", cmd);
        
        switch (cmd) {
        case CMD_START_STREAM:
            if (!streaming_active) {
                LOG_INF("Starting data stream");
                streaming_active = true;
                k_work_schedule(&streaming_work, K_NO_WAIT);
            }
            break;
            
        case CMD_STOP_STREAM:
            if (streaming_active) {
                LOG_INF("Stopping data stream");
                streaming_active = false;
                k_work_cancel_delayable(&streaming_work);
            }
            break;
            
        case CMD_MIPE_SYNC:
            LOG_INF("Mipe sync command received - sending simulated Mipe status");
            /* Send simulated Mipe status data */
            {
                uint8_t status_packet[16] = {0};
                /* Simulated Mipe status:
                 * Byte 0: Connection state (2 = connected)
                 * Byte 1: RSSI (-65 dBm)
                 * Byte 2-7: Device address (simulated)
                 * Byte 8-11: Connection duration (simulated 120 seconds)
                 * Byte 12-15: Battery voltage (3.7V as float)
                 */
                status_packet[0] = 2;  /* Connected */
                status_packet[1] = -65;  /* RSSI */
                /* Simulated MAC address */
                status_packet[2] = 0xAA;
                status_packet[3] = 0xBB;
                status_packet[4] = 0xCC;
                status_packet[5] = 0xDD;
                status_packet[6] = 0xEE;
                status_packet[7] = 0xFF;
                /* Connection duration: 120 seconds */
                uint32_t duration = 120;
                status_packet[8] = (duration >> 0) & 0xFF;
                status_packet[9] = (duration >> 8) & 0xFF;
                status_packet[10] = (duration >> 16) & 0xFF;
                status_packet[11] = (duration >> 24) & 0xFF;
                /* Battery voltage: 3.7V */
                float battery = 3.7f;
                memcpy(&status_packet[12], &battery, sizeof(float));
                
                /* Send via Mipe status characteristic */
                send_mipe_status_notification(status_packet, sizeof(status_packet));
            }
            break;
            
        default:
            LOG_WRN("Unknown command: 0x%02x", cmd);
            break;
        }
    }
    return len;
}

static const struct bt_gatt_attr custom_service_attrs[] = {
    BT_GATT_PRIMARY_SERVICE(BT_UUID_CUSTOM_SERVICE),
    /* Measurement Data Characteristic (RSSI Data) */
    BT_GATT_CHARACTERISTIC(BT_UUID_MEASUREMENT_DATA_CHAR,
                           BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE, NULL, NULL, &measurement_data),
    BT_GATT_CCC(on_measurement_data_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    /* Control Command Characteristic */
    BT_GATT_CHARACTERISTIC(BT_UUID_CONTROL_COMMAND_CHAR,
                           BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_WRITE, NULL, on_control_command_write, &control_command),
    /* System Status Characteristic */
    BT_GATT_CHARACTERISTIC(BT_UUID_SYSTEM_STATUS_CHAR,
                           BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE, NULL, NULL, &system_status),
    BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    /* Mipe Status Characteristic */
    BT_GATT_CHARACTERISTIC(BT_UUID_MIPE_STATUS_CHAR,
                           BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE, NULL, NULL, &mipe_status),
    BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    /* Log Data Characteristic */
    BT_GATT_CHARACTERISTIC(BT_UUID_LOG_DATA_CHAR,
                           BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE, NULL, NULL, &log_data),
    BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
};

BT_GATT_SERVICE_DEFINE(custom_service, custom_service_attrs);

int ble_service_init(void)
{
    /* Initialize the streaming work handler */
    k_work_init_delayable(&streaming_work, streaming_work_handler);
    
    /* The service is registered automatically by the BT_GATT_SERVICE_DEFINE macro */
    LOG_INF("BLE service initialized");
    return 0;
}

int ble_service_send_measurement_data(const uint8_t *data, uint16_t len)
{
    if (!app_conn) {
        return -ENOTCONN;
    }

    return bt_gatt_notify(app_conn, &custom_service.attrs[2], data, len);
}

void ble_service_set_app_conn(struct bt_conn *conn)
{
    app_conn = conn;
}

/* Helper function to send Mipe status notifications */
static int send_mipe_status_notification(const uint8_t *data, uint16_t len)
{
    if (!app_conn) {
        return -ENOTCONN;
    }

    /* Mipe status characteristic value is at index 10 in the service attributes */
    return bt_gatt_notify(app_conn, &custom_service.attrs[10], data, len);
}
