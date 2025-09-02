/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * BLE Central REAL VERSION - BEACON MODE
 * Scans for Mipe device advertising packets to read RSSI for distance measurement
 * No BLE connection is established - Mipe is treated as a beacon only
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "ble_central.h"

LOG_MODULE_REGISTER(ble_central_real, LOG_LEVEL_INF);

/* Target device name */
#define MIPE_DEVICE_NAME "SinglePing Mipe"

/* Beacon timeout in seconds */
#define BEACON_TIMEOUT_SEC 10

/* Callback management */
static mipe_rssi_cb_t mipe_rssi_callback = NULL;
static bool scanning = false;

/* Mipe tracking */
static bool mipe_detected = false;
static uint32_t last_mipe_seen = 0;
static uint32_t mipe_packet_count = 0;
static bt_addr_le_t mipe_addr = {0};
static uint16_t mipe_battery_mv = 0;  /* Battery voltage from advertising */
static bool battery_initialized = false;  /* Track if battery has been set */

/* Context for the data parser */
struct ad_parse_ctx {
    const bt_addr_le_t *addr;
    int8_t rssi;
    bool found_mipe;
    uint16_t battery_mv;
};

/* Data parsing callback */
static bool ad_parse_cb(struct bt_data *data, void *user_data)
{
    struct ad_parse_ctx *ctx = user_data;
    
    /* Debug log all data types received */
    LOG_DBG("AD data type: 0x%02X, len: %u", data->type, data->data_len);
    
    /* Check for device name */
    if (data->type == BT_DATA_NAME_COMPLETE || data->type == BT_DATA_NAME_SHORTENED) {
        char name[32];
        uint8_t name_len;
        
        name_len = MIN(data->data_len, sizeof(name) - 1);
        memcpy(name, data->data, name_len);
        name[name_len] = '\0';
        
        if (strcmp(name, MIPE_DEVICE_NAME) == 0) {
            ctx->found_mipe = true;
            LOG_DBG("Found Mipe device: %s", name);
        }
    }
    /* Check for manufacturer data (contains battery voltage) */
    else if (data->type == BT_DATA_MANUFACTURER_DATA) {
        LOG_DBG("Manufacturer data found, len: %u", data->data_len);
        if (data->data_len >= 4) {
            /* Only process our company ID (0xFFFF) */
            uint16_t company_id = data->data[0] | (data->data[1] << 8);
            if (company_id == 0xFFFF) {
                /* Extract battery voltage from manufacturer data */
                /* Format: [Company ID (2 bytes)] [Battery mV (2 bytes)] */
                ctx->battery_mv = data->data[2] | (data->data[3] << 8);
                
                /* Store battery value immediately in static variable */
                /* This ensures we capture battery even if it comes in a different packet than the name */
                if (ctx->battery_mv > 0) {
                    mipe_battery_mv = ctx->battery_mv;
                    battery_initialized = true;
                    /* LOG_INF("Mipe battery stored directly: %u mV", mipe_battery_mv); */
                }
                
                /* Only log if battery value is different to reduce duplicate logs */
                static uint16_t last_logged_battery = 0;
                if (ctx->battery_mv != last_logged_battery) {
                    /* LOG_INF("Mipe battery updated: %u mV", ctx->battery_mv); */
                    last_logged_battery = ctx->battery_mv;
                }
            } else {
                LOG_DBG("Ignoring manufacturer data from company 0x%04X", company_id);
            }
        } else {
            LOG_DBG("Manufacturer data too short: %u bytes", data->data_len);
        }
    }
    
    return true; /* Continue parsing to get all data */
}

/* Scan callback */
static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                        struct net_buf_simple *ad)
{
    /* Accept ALL advertising types including non-connectable beacons */
    /* This includes:
     * - BT_GAP_ADV_TYPE_ADV_IND (0x00) - Connectable undirected
     * - BT_GAP_ADV_TYPE_ADV_DIRECT_IND (0x01) - Connectable directed
     * - BT_GAP_ADV_TYPE_ADV_SCAN_IND (0x02) - Scannable undirected
     * - BT_GAP_ADV_TYPE_ADV_NONCONN_IND (0x03) - Non-connectable undirected (BEACONS!)
     * - BT_GAP_ADV_TYPE_SCAN_RSP (0x04) - Scan response
     */
    
    struct ad_parse_ctx ctx = {
        .addr = addr,
        .rssi = rssi,
        .found_mipe = false,
        .battery_mv = 0,
    };

    /* Parse advertising data */
    bt_data_parse(ad, ad_parse_cb, &ctx);
    
    /* Process if this is a Mipe device */
    if (ctx.found_mipe) {
        char addr_str[BT_ADDR_LE_STR_LEN];
        bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
        
        /* Track Mipe detection */
        if (!mipe_detected) {
            LOG_INF("*** SinglePing Mipe DETECTED at %s ***", addr_str);
            LOG_INF("Connection to Mipe: CONNECTED (Beacon Mode)");
            LOG_INF("Initial battery: %u mV (initialized: %s)", 
                    mipe_battery_mv, battery_initialized ? "yes" : "no");
            mipe_detected = true;
            bt_addr_le_copy(&mipe_addr, addr);
        }
        
        /* Update tracking */
        last_mipe_seen = k_uptime_get_32();
        mipe_packet_count++;
        
        /* LOG_INF("Mipe: RSSI=%d dBm", rssi); */
        
        /* Forward RSSI to callback immediately */
        if (mipe_rssi_callback) {
            mipe_rssi_callback(rssi, k_uptime_get_32());
        }
    }
}

/* Check for beacon timeout */
static void check_beacon_timeout(void)
{
    if (mipe_detected) {
        uint32_t now = k_uptime_get_32();
        uint32_t elapsed_ms = now - last_mipe_seen;
        
        if (elapsed_ms > (BEACON_TIMEOUT_SEC * 1000)) {
            char addr_str[BT_ADDR_LE_STR_LEN];
            bt_addr_le_to_str(&mipe_addr, addr_str, sizeof(addr_str));
            
            LOG_WRN("*** SinglePing Mipe LOST (timeout after %u ms) ***", elapsed_ms);
            LOG_INF("Connection to Mipe: DISCONNECTED");
            LOG_INF("Last known address: %s, Total packets received: %u", 
                    addr_str, mipe_packet_count);
            
            mipe_detected = false;
            mipe_packet_count = 0;
            memset(&mipe_addr, 0, sizeof(mipe_addr));
            /* Don't reset battery value on timeout - keep last known value */
            LOG_INF("Keeping last known battery value: %u mV", mipe_battery_mv);
        }
    }
}

int ble_central_init(mipe_rssi_cb_t rssi_cb)
{
    if (!rssi_cb) {
        LOG_ERR("Invalid callback provided");
        return -EINVAL;
    }

    mipe_rssi_callback = rssi_cb;

    LOG_INF("BLE Central BEACON MODE initialized");
    return 0;
}

int ble_central_start_scan(void)
{
    int err;
    
    if (scanning) {
        LOG_WRN("Already scanning");
        return 0;
    }

    struct bt_le_scan_param scan_param = {
        .type = BT_LE_SCAN_TYPE_ACTIVE,
        .options = BT_LE_SCAN_OPT_NONE,
        .interval = BT_GAP_SCAN_FAST_INTERVAL,
        .window = BT_GAP_SCAN_FAST_WINDOW,
    };

    LOG_INF("=== Starting scan for SinglePing Mipe ===");
    LOG_INF("Accepting ALL advertising types (including non-connectable beacons)");
    
    err = bt_le_scan_start(&scan_param, device_found);
    if (err) {
        LOG_ERR("Failed to start scan: %d", err);
        return err;
    }

    scanning = true;
    LOG_INF("Scanning ACTIVE - Looking for: %s", MIPE_DEVICE_NAME);
    return 0;
}

int ble_central_stop_scan(void)
{
    int err;
    
    if (!scanning) {
        return 0;
    }

    err = bt_le_scan_stop();
    if (err) {
        LOG_ERR("Failed to stop scan: %d", err);
        return err;
    }

    scanning = false;
    LOG_INF("Stopped scanning");
    return 0;
}

bool ble_central_is_scanning(void)
{
    /* Also check for timeout while we're at it */
    if (scanning) {
        check_beacon_timeout();
    }
    return scanning;
}

bool ble_central_is_mipe_detected(void)
{
    return mipe_detected;
}

uint32_t ble_central_get_mipe_packet_count(void)
{
    return mipe_packet_count;
}

uint16_t ble_central_get_mipe_battery_mv(void)
{
    /* Debug log to track battery value retrieval */
    /* LOG_INF("ble_central_get_mipe_battery_mv() called, returning: %u mV (initialized: %s)", 
            mipe_battery_mv, battery_initialized ? "yes" : "no"); */
    return mipe_battery_mv;
}
