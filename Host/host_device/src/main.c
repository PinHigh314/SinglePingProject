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
static uint16_t latest_mipe_battery_mv = 0;

/* Mipe status data */
static mipe_status_t mipe_status = {0};
static uint16_t mipe_connection_attempts = 0;

/* Host battery monitoring - placeholder constant value */
uint16_t get_host_battery_mv(void)
{
    /* TODO: Implement actual ADC reading for host battery */
    /* For now, return a constant test value as requested */
    static uint32_t last_log = 0;
    uint32_t now = k_uptime_get_32();
    
    /* Log periodically for debugging */
    if (now - last_log > 10000) {  /* Every 10 seconds */
        LOG_INF("Host battery reading: 7654 mV (constant test value)");
        last_log = now;
    }
    
    return 7654;  /* 7.654V constant as requested */
}

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

    LOG_DBG("Updating mipe status - battery: %.2fV", (double)temp_status.battery_voltage);
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
    /* Update latest values */
    latest_mipe_rssi = rssi;
    latest_mipe_timestamp = timestamp;
    
    /* Track connection state changes */
    if (!mipe_connected) {
        mipe_connected = true;
        /* Store the battery value on first detection */
        latest_mipe_battery_mv = ble_central_get_mipe_battery_mv();
        LOG_INF("=== Mipe Connection State Change ===");
        LOG_INF("Connection to Mipe: CONNECTED");
        LOG_INF("Initial RSSI: %d dBm", rssi);
        LOG_INF("Mipe battery: %u mV", latest_mipe_battery_mv);
        /* Don't send log_ble here - it causes buffer conflicts */
        /* Don't update status immediately - it causes buffer conflicts during scan */
        /* Status will be updated on next RSSI packet or via MIPE_SYNC command */
        
        /* Skip sending the first RSSI packet to avoid buffer conflicts */
        return;
    }
    
    /* Send RSSI to app in real-time if streaming is active */
    if (streaming_active && motoapp_connected) {
        /* Get battery values for logging */
        uint16_t host_batt = get_host_battery_mv();
        /* Use the stored battery value instead of calling the function each time */
        uint16_t mipe_batt = latest_mipe_battery_mv;
        
        int err = ble_peripheral_send_rssi_data(rssi, 0);
        if (err == 0) {
            /* Successfully sent - log periodically to avoid spam */
            static uint32_t last_success_log = 0;
            static uint32_t send_count = 0;
            send_count++;
            if (timestamp - last_success_log > 5000) {
                LOG_INF("Real-time bundle sent #%u: RSSI=%d dBm, Host=%u mV, Mipe=%u mV", 
                        send_count, rssi, host_batt, mipe_batt);
                last_success_log = timestamp;
            }
        } else if (err != -EAGAIN) {
            /* Log errors except buffer full */
            static uint32_t last_error_log = 0;
            if (timestamp - last_error_log > 1000) {
                LOG_WRN("Failed to send RSSI bundle: %d", err);
                last_error_log = timestamp;
            }
        }
    }
    
    /* Log RSSI periodically (every 5 seconds) */
    static uint32_t last_rssi_log = 0;
    if (timestamp - last_rssi_log > 5000) {
        LOG_INF("Mipe RSSI: %d dBm (stable)", rssi);
        last_rssi_log = timestamp;
    }
}

/* Data transmission callback - NOT USED in real-time mode */
static int get_rssi_data(int8_t *rssi, uint32_t *timestamp)
{
    /* This callback is no longer used in real-time mode */
    /* RSSI is sent directly from mipe_rssi_received() */
    return -ENODATA;
}

int main(void)
{
    int err;

    LOG_INF("=== Host Device Starting (Minimal) ===");
    LOG_INF("Initial mipe_connected state: %s", mipe_connected ? "true" : "false");
    LOG_INF("Host battery monitoring: Using constant 7654 mV for testing");

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

    /* Don't start scanning automatically - wait for app to request streaming */
    LOG_INF("=== Host Device Ready ===");
    LOG_INF("Advertising as: MIPE_HOST_A1B2");
    LOG_INF("Waiting for app to start streaming before scanning for Mipe");

    /* Main loop */
    while (1) {
        k_sleep(K_SECONDS(5));
        
        /* Check Mipe connection state */
        bool mipe_currently_detected = ble_central_is_mipe_detected();
        
        /* Handle disconnection */
        if (mipe_connected && !mipe_currently_detected) {
            mipe_connected = false;
            LOG_WRN("=== Mipe Connection State Change ===");
            LOG_INF("Connection to Mipe: DISCONNECTED");
            LOG_INF("Last known RSSI: %d dBm", latest_mipe_rssi);
            log_ble("Mipe beacon lost - timeout");
            latest_mipe_rssi = -70;  /* Reset to default */
        }
        
        /* Log periodic status if Mipe is connected */
        if (mipe_connected) {
            uint32_t packet_count = ble_central_get_mipe_packet_count();
            uint32_t uptime_sec = k_uptime_get_32() / 1000;
            LOG_INF("Mipe Status: Connected for %u sec, %u packets received, RSSI: %d dBm", 
                    uptime_sec, packet_count, latest_mipe_rssi);
        }
        
        /* NOTE: Mipe status is ONLY sent when explicitly requested via MIPE_SYNC command */
        /* No automatic status updates here */
    }

    return 0;
}
