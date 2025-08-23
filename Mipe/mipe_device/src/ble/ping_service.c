#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <string.h>

#include "ping_service.h"

LOG_MODULE_REGISTER(ping_service, LOG_LEVEL_INF);

/* Custom UUID for SinglePing service */
#define BT_UUID_SINGLEPING_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)

#define BT_UUID_SINGLEPING_PING_CHAR_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1)

#define BT_UUID_SINGLEPING_RESPONSE_CHAR_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef2)

#define BT_UUID_SINGLEPING_BATTERY_CHAR_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef3)

static struct bt_uuid_128 singleping_service_uuid = BT_UUID_INIT_128(BT_UUID_SINGLEPING_SERVICE_VAL);
static struct bt_uuid_128 ping_char_uuid = BT_UUID_INIT_128(BT_UUID_SINGLEPING_PING_CHAR_VAL);
static struct bt_uuid_128 response_char_uuid = BT_UUID_INIT_128(BT_UUID_SINGLEPING_RESPONSE_CHAR_VAL);
static struct bt_uuid_128 battery_char_uuid = BT_UUID_INIT_128(BT_UUID_SINGLEPING_BATTERY_CHAR_VAL);

static ping_request_cb_t request_callback = NULL;
static struct bt_gatt_indicate_params indicate_params;
static uint8_t response_data[247];
static uint16_t response_len = 0;
static uint16_t battery_voltage_mv = 0;

/* GATT characteristic callbacks */
static ssize_t ping_char_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                              const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    const uint8_t *data = buf;

    LOG_DBG("Ping characteristic write, len: %u", len);

    if (request_callback) {
        request_callback(data, len);
    }

    return len;
}

static ssize_t response_char_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                 void *buf, uint16_t len, uint16_t offset)
{
    LOG_DBG("Response characteristic read, len: %u", response_len);
    
    return bt_gatt_attr_read(conn, attr, buf, len, offset, response_data, response_len);
}

static void response_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    LOG_DBG("Response CCC changed: %s", value == BT_GATT_CCC_INDICATE ? "indicate" : "disabled");
}

static ssize_t battery_char_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                void *buf, uint16_t len, uint16_t offset)
{
    LOG_DBG("Battery characteristic read, voltage: %u mV", battery_voltage_mv);
    
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &battery_voltage_mv, sizeof(battery_voltage_mv));
}

static void battery_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    LOG_DBG("Battery CCC changed: %s", value == BT_GATT_CCC_NOTIFY ? "notify" : "disabled");
}

/* GATT service definition */
BT_GATT_SERVICE_DEFINE(singleping_service,
    BT_GATT_PRIMARY_SERVICE(&singleping_service_uuid),
    
    /* Ping characteristic - write only */
    BT_GATT_CHARACTERISTIC(&ping_char_uuid.uuid,
                          BT_GATT_CHRC_WRITE,
                          BT_GATT_PERM_WRITE,
                          NULL, ping_char_write, NULL),
    
    /* Response characteristic - read/indicate */
    BT_GATT_CHARACTERISTIC(&response_char_uuid.uuid,
                          BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
                          BT_GATT_PERM_READ,
                          response_char_read, NULL, NULL),
    BT_GATT_CCC(response_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    
    /* Battery voltage characteristic - read/notify */
    BT_GATT_CHARACTERISTIC(&battery_char_uuid.uuid,
                          BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                          BT_GATT_PERM_READ,
                          battery_char_read, NULL, NULL),
    BT_GATT_CCC(battery_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static void indicate_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err)
{
    LOG_DBG("Indication %s", err != 0U ? "fail" : "success");
}

int ping_service_init(ping_request_cb_t request_cb)
{
    request_callback = request_cb;
    
    indicate_params.attr = &singleping_service.attrs[3]; /* Response characteristic */
    indicate_params.func = indicate_cb;
    
    LOG_INF("Ping service initialized");
    return 0;
}

int ping_service_send_response(const uint8_t *data, uint16_t len)
{
    int err;
    struct bt_conn *conn;

    if (len > sizeof(response_data)) {
        LOG_ERR("Response data too large: %u", len);
        return -EINVAL;
    }

    /* Copy response data */
    memcpy(response_data, data, len);
    response_len = len;

    /* Get current connection */
    /* Get the current connection from the connection manager */
    extern struct bt_conn *current_conn; /* Declared in ble_peripheral.c */
    conn = current_conn;
    if (!conn) {
        LOG_ERR("No active connection");
        return -ENOTCONN;
    }

    /* Send indication */
    indicate_params.data = response_data;
    indicate_params.len = response_len;

    err = bt_gatt_indicate(conn, &indicate_params);
    if (err) {
        LOG_ERR("Failed to send indication: %d", err);
    } else {
        LOG_DBG("Response indication sent, len: %u", len);
    }

    return err;
}

int ping_service_update_battery_voltage(uint16_t voltage_mv)
{
    int err;
    struct bt_conn *conn;
    
    /* Update stored value */
    battery_voltage_mv = voltage_mv;
    
    /* Get current connection */
    extern struct bt_conn *current_conn;
    conn = current_conn;
    if (!conn) {
        LOG_DBG("No active connection, battery voltage updated to %u mV", voltage_mv);
        return 0; /* Not an error, just no connection */
    }
    
    /* Send notification if enabled */
    err = bt_gatt_notify(conn, &singleping_service.attrs[6], &battery_voltage_mv, sizeof(battery_voltage_mv));
    if (err) {
        LOG_ERR("Failed to send battery notification: %d", err);
    } else {
        LOG_DBG("Battery voltage notification sent: %u mV", voltage_mv);
    }
    
    return err;
}
