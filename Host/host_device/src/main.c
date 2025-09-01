/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Host Device Application - Minimal Version
 * - RSSI measurement from Mipe beacons
 * - BLE communication with MotoApp
 * - Clean, focused implementation without LED indicators
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "ble/ble_peripheral.h"
#include "ble/ble_central.h"

LOG_MODULE_REGISTER(host_main, LOG_LEVEL_WRN);

/* BLE logging buffer */
static char ble_log_buffer[128];

/* Connection states */
static bool motoapp_connected = false;
static bool mipe_connected = false;
static bool streaming_active = false;

/* Latest RSSI from Mipe */
static int8_t latest_mipe_rssi = -70;
static uint32_t latest_mipe_timestamp = 0;

/* Mipe status data */
static mipe_status_t mipe_status = {0};
static uint16_t mipe_connection_attempts = 0;

/* Helper function to send logs via BLE */
static void log_ble(const char *format, ...)
{
    va_list args;
    int err;
    
    va_start(args, format);
    vsnprintf(ble_log_buffer, sizeof(ble_log_buffer), format, args);
    va_end(args);

    LOG_INF("%s", ble_log_buffer);
    
    /* Only try to send via BLE if we're connected and ready */
    if (motoapp_connected) {
        err = ble_peripheral_send_log_data(ble_log_buffer);
        if (err == -EAGAIN) {
            /* Buffer full, just log locally */
            LOG_DBG("BLE log buffer full, skipping");
        }
    }
}

static void update_mipe_status(void)
{
    /* Only update dynamic fields, preserve battery voltage and other static data */
    mipe_status_t temp_status = mipe_status;
    
    /* Update only the dynamic fields */
    temp_status.status_flags = (ble_central_is_scanning() ? 1 : 0) | (mipe_connected ? 2 : 0);
    temp_status.rssi = mipe_connected ? latest_mipe_rssi : 0;
    temp_status.last_scan_timestamp = k_uptime_get_32();
    temp_status.connection_attempts = mipe_connection_attempts;

    LOG_DBG("Updating mipe status - battery: %.2fV", temp_status.battery_voltage);
    ble_peripheral_update_mipe_status(&temp_status);
}

/* Mipe sync operation */
static void handle_mipe_sync(void)
{
    log_ble("=== MIPE SYNC STARTED ===");
    
    /* TODO: Implement actual Mipe connection logic here */
    /* For now, simulate a successful sync with mock data */
    k_sleep(K_MSEC(2000));  /* 2000ms timeout */
    
    /* Use constant battery value for testing Host-to-App communication */
    mipe_status.battery_voltage = 3.30f;  /* Constant test value */
    mipe_status.connection_duration = 2;  /* 2 second connection */
    strncpy(mipe_status.connection_state, "Connected", sizeof(mipe_status.connection_state) - 1);
    mipe_status.connection_state[sizeof(mipe_status.connection_state) - 1] = '\0';
    strncpy(mipe_status.device_address, "AA:BB:CC:DD:EE:FF", sizeof(mipe_status.device_address) - 1);
    mipe_status.device_address[sizeof(mipe_status.device_address) - 1] = '\0';
    
    log_ble("MIPE SYNC COMPLETE");
    log_ble("Battery: 3.30v (constant test), Duration: 2s");
    
    /* Update status to reflect sync completion */
    mipe_status_t sync_status = mipe_status;
    ble_peripheral_update_mipe_status(&sync_status);
}

/* BLE Peripheral callbacks */
static void app_connected(void)
{
    motoapp_connected = true;
    
    /* Wait for connection to stabilize before sending logs */
    k_sleep(K_MSEC(500));
    log_ble("MotoApp connected");
}

static void app_disconnected(void)
{
    log_ble("MotoApp disconnected");
    motoapp_connected = false;
    streaming_active = false;
}

static void streaming_state_changed(bool active)
{
    log_ble("Streaming %s", active ? "started" : "stopped");
    streaming_active = active;
}

/* BLE Central callback */
static void mipe_rssi_received(int8_t rssi, uint32_t timestamp)
{
    /* Don't log every RSSI update - too frequent */
    LOG_DBG("Mipe RSSI: %d dBm at %u ms", rssi, timestamp);
    latest_mipe_rssi = rssi;
    latest_mipe_timestamp = timestamp;
    
    /* In beacon mode, we're always "connected" for RSSI purposes */
    if (!mipe_connected) {
        mipe_connected = true;
        log_ble("Mipe beacon detected - RSSI streaming active");
        update_mipe_status();
    }
}

/* Data transmission callback */
static int get_rssi_data(int8_t *rssi, uint32_t *timestamp)
{
    if (!streaming_active) {
        return -ENODATA;
    }
    
    /* Only send data if we have a real RSSI from Mipe */
    if (mipe_connected && latest_mipe_timestamp > 0) {
        /* Use actual Mipe RSSI */
        int8_t rssi_to_send = latest_mipe_rssi;
        /* Don't log every transmission - too frequent at 10Hz */
        LOG_DBG("TX RSSI: %d dBm (from Mipe)", rssi_to_send);
        
        *rssi = rssi_to_send;
        *timestamp = k_uptime_get_32();
        
        return 0;
    } else {
        /* No Mipe beacon detected - don't send any data */
        LOG_DBG("No Mipe beacon detected - skipping RSSI transmission");
        return -ENODATA;
    }
}

int main(void)
{
    int err;

    LOG_INF("=== Host Device Starting (Minimal) ===");
    LOG_INF("Initial mipe_connected state: %s", mipe_connected ? "true" : "false");

    /* Initialize Bluetooth */
    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return err;
    }
    LOG_INF("Bluetooth initialized");

    /* Initialize BLE Peripheral (for MotoApp connection) */
    err = ble_peripheral_init(app_connected, app_disconnected, 
                             streaming_state_changed, get_rssi_data,
                             handle_mipe_sync);
    if (err) {
        LOG_ERR("Failed to initialize BLE peripheral: %d", err);
        return err;
    }

    /* Initialize BLE Central (for Mipe beacon scanning) */
    err = ble_central_init(mipe_rssi_received);
    if (err) {
        LOG_ERR("Failed to initialize BLE central: %d", err);
        return err;
    }

    /* Start advertising */
    err = ble_peripheral_start_advertising();
    if (err) {
        LOG_ERR("Failed to start advertising: %d", err);
        return err;
    }

    /* Start scanning for Mipe */
    err = ble_central_start_scan();
    if (err) {
        LOG_ERR("Failed to start scanning: %d", err);
        /* Continue anyway - scanning can be retried */
    }

    LOG_INF("=== Host Device Ready ===");
    LOG_INF("Advertising as: MIPE_HOST_A1B2");
    LOG_INF("Scanning for: SinglePing Mipe");

    /* Main loop */
    while (1) {
        /* Update Mipe status less frequently to avoid buffer overflow */
        k_sleep(K_SECONDS(5));
        
        /* Only update status if MotoApp is connected */
        if (motoapp_connected) {
            update_mipe_status();
        }
    }

    return 0;
}
