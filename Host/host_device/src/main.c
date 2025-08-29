/*
 * Host Device - nRF54L15DK
 * BLE Dual-Role Application
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/hci.h>
#include "ble_service.h"

LOG_MODULE_REGISTER(host_main, LOG_LEVEL_INF);

static struct bt_conn *app_conn = NULL;
static struct bt_conn *mipe_conn = NULL;

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SERVICE_VAL),
};

static const struct bt_le_adv_param adv_params = {
    .options = BT_LE_ADV_OPT_CONN, // Use the new, non-deprecated flag
    .interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
    .interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
    .peer = NULL,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (err) {
        LOG_ERR("Connection failed (err %u)", err);
        return;
    }

    // Distinguish between Mipe and App connections
    struct bt_conn_info info;
    bt_conn_get_info(conn, &info);
    if (info.role == BT_CONN_ROLE_CENTRAL) {
        mipe_conn = bt_conn_ref(conn);
        LOG_INF("Mipe connected: %s", addr);
    } else {
        app_conn = bt_conn_ref(conn);
        ble_service_set_app_conn(app_conn);
        LOG_INF("MotoApp connected: %s", addr);
        
        /* Request connection parameter update for stability */
        struct bt_le_conn_param param = {
            .interval_min = BT_GAP_INIT_CONN_INT_MIN,  /* 30ms */
            .interval_max = BT_GAP_INIT_CONN_INT_MAX,  /* 50ms */
            .latency = 0,
            .timeout = 400,  /* 4 seconds */
        };
        
        err = bt_conn_le_param_update(conn, &param);
        if (err) {
            LOG_WRN("Failed to request connection parameter update: %d", err);
        } else {
            LOG_INF("Connection parameter update requested");
        }
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Disconnected %s (reason %u)", addr, reason);

    if (conn == app_conn) {
        bt_conn_unref(app_conn);
        app_conn = NULL;
        ble_service_set_app_conn(NULL);
        
        /* Automatically restart advertising for revolving connection support */
        LOG_INF("Restarting advertising for MotoApp discovery");
        int err = bt_le_adv_start(&adv_params, ad, ARRAY_SIZE(ad), NULL, 0);
        if (err) {
            LOG_ERR("Failed to restart advertising (err %d)", err);
            /* Try again after a delay */
            k_msleep(1000);
            err = bt_le_adv_start(&adv_params, ad, ARRAY_SIZE(ad), NULL, 0);
            if (err) {
                LOG_ERR("Second attempt to restart advertising failed (err %d)", err);
            }
        } else {
            LOG_INF("Advertising restarted successfully");
        }
    } else if (conn == mipe_conn) {
        bt_conn_unref(mipe_conn);
        mipe_conn = NULL;
    }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

int main(void)
{
    int err;

    LOG_INF("=== Host Device (Dual-Role) Starting ===");

    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return 0;
    }
    LOG_INF("Bluetooth initialized");

    err = ble_service_init();
    if (err) {
        LOG_ERR("Failed to initialize BLE service: %d", err);
        return 0;
    }

    err = bt_le_adv_start(&adv_params, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return 0;
    }
    LOG_INF("Advertising successfully started as %s", CONFIG_BT_DEVICE_NAME);

    // TODO: Add scanning logic for Mipe device

    while (1) {
        k_msleep(1000);
    }
    return 0;
}
