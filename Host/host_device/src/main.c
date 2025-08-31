/*
 * Host Device - nRF54L15DK
 * BLE Central Application - Simplified Working Version
 * 
 * This device acts as a BLE central device that can:
 * - Scan for BLE devices
 * - Connect to devices
 * - Handle basic BLE communication
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include "ble_service.h"

LOG_MODULE_REGISTER(host_main, LOG_LEVEL_INF);

static struct bt_conn *app_conn = NULL;

/* Basic advertising data */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, strlen(CONFIG_BT_DEVICE_NAME)),
};

static const struct bt_le_adv_param adv_params = {
    .options = BT_LE_ADV_OPT_CONN,
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

    app_conn = bt_conn_ref(conn);
    LOG_INF("=== DEVICE CONNECTED ===");
    LOG_INF("Address: %s", addr);
    LOG_INF("Connection established successfully");
    
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

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Disconnected %s (reason %u)", addr, reason);

    if (conn == app_conn) {
        bt_conn_unref(app_conn);
        app_conn = NULL;
        
        LOG_INF("=== DEVICE DISCONNECTED ===");
        LOG_INF("Reason: 0x%02x", reason);
        
        /* Automatically restart advertising */
        LOG_INF("Restarting advertising for device discovery");
        int err = bt_le_adv_start(&adv_params, ad, ARRAY_SIZE(ad), NULL, 0);
        if (err) {
            LOG_ERR("Failed to restart advertising (err %d)", err);
            /* Try again after a delay */
            k_msleep(1000);
            err = bt_le_adv_start(&adv_params, ad, ARRAY_SIZE(ad), NULL, 0);
            if (err) {
                LOG_ERR("Second attempt to restart advertising failed (err %d)", err);
            }
        }
    }
}

static void bt_ready(void)
{
    int err;

    LOG_INF("Bluetooth initialized");

    /* Start advertising */
    err = bt_le_adv_start(&adv_params, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return;
    }

    LOG_INF("Advertising started - Device name: %s", CONFIG_BT_DEVICE_NAME);
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Passkey for %s: %06u", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Pairing cancelled: %s", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
    .passkey_display = auth_passkey_display,
    .passkey_entry = NULL,
    .cancel = auth_cancel,
};

static void scan_cb(const struct bt_le_scan_recv_info *info,
                    struct bt_le_scan_recv_info *scan_info)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(info->addr, addr, sizeof(addr));

    LOG_INF("Device found: %s", addr);
    LOG_INF("  RSSI: %d", info->rssi);
    LOG_INF("  Type: %u", info->addr->type);
}

static struct bt_le_scan_cb scan_callbacks = {
    .recv = scan_cb,
};

int main(void)
{
    int err;

    LOG_INF("Starting Host Device - BLE Central Application");
    LOG_INF("Board: nRF54L15DK");
    LOG_INF("MCU: nRF54L15 (ARM Cortex-M33)");
    LOG_INF("Zephyr Version: %s", KERNEL_VERSION_STRING);

    /* Initialize BLE service */
    err = ble_service_init();
    if (err) {
        LOG_ERR("BLE service initialization failed: %d", err);
        return -1;
    }

    /* Initialize Bluetooth */
    err = bt_enable(&bt_ready);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return -1;
    }

    /* Set authentication callbacks */
    bt_conn_auth_cb_register(&auth_cb_display);

    /* Set scan callbacks */
    bt_le_scan_cb_register(&scan_callbacks);

    LOG_INF("Host device initialization complete");
    LOG_INF("Starting main application loop...");

    /* Main application loop */
    uint32_t counter = 0;
    
    while (1) {
        /* Periodic status logging */
        if (counter % 100 == 0) {
            LOG_INF("System running - Counter: %u", counter);
            if (ble_service_is_connected()) {
                LOG_INF("BLE service: Connected");
            } else {
                LOG_INF("BLE service: Not connected");
            }
        }
        
        counter++;
        k_msleep(1000);  // 1 second delay
    }

    return 0;
}
