#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/addr.h>

#include "ble_peripheral.h"

LOG_MODULE_REGISTER(ble_peripheral, LOG_LEVEL_INF);

static connection_status_cb_t connection_callback = NULL;
struct bt_conn *current_conn = NULL;  /* Make this non-static so ping_service can access it */

/* Advertising data */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

/* Connection callbacks */
static void connected(struct bt_conn *conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (err) {
        LOG_ERR("Failed to connect to %s (%u)", addr, err);
        return;
    }

    LOG_INF("Connected %s", addr);
    current_conn = bt_conn_ref(conn);

    if (connection_callback) {
        connection_callback(true);
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Disconnected from %s (reason 0x%02x)", addr, reason);

    if (current_conn) {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }

    if (connection_callback) {
        connection_callback(false);
    }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

int ble_peripheral_init(connection_status_cb_t conn_cb)
{
    int err;

    connection_callback = conn_cb;

    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return err;
    }

    LOG_INF("Bluetooth initialized");
    return 0;
}

int ble_peripheral_start_advertising(void)
{
    int err;

    struct bt_le_adv_param adv_param = {
        .id = BT_ID_DEFAULT,
        .sid = 0,
        .secondary_max_skip = 0,
        .options = BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_USE_NAME,
        .interval_min = BT_GAP_ADV_FAST_INT_MIN_1,  /* 100ms intervals */
        .interval_max = BT_GAP_ADV_FAST_INT_MAX_1,  /* 100ms intervals */
        .peer = NULL,
    };
    
    err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return err;
    }

    LOG_INF("Advertising successfully started");
    return 0;
}

int ble_peripheral_stop_advertising(void)
{
    int err;

    err = bt_le_adv_stop();
    if (err) {
        LOG_ERR("Advertising failed to stop (err %d)", err);
        return err;
    }

    LOG_INF("Advertising stopped");
    return 0;
}

bool ble_peripheral_is_connected(void)
{
    return (current_conn != NULL);
}
