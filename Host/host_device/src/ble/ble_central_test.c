/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * BLE Central TEST VERSION for v4
 * Modified to generate simulated RSSI without actual Mipe connection
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "ble_central.h"

LOG_MODULE_REGISTER(ble_central, LOG_LEVEL_INF);

/* Connection and callback management */
static struct bt_conn *mipe_conn = NULL;
static mipe_connection_cb_t mipe_conn_callback = NULL;
static mipe_rssi_cb_t mipe_rssi_callback = NULL;
static bool scanning = false;
static bool test_mode_active = false;

/* RSSI measurement timer and work */
static struct k_timer rssi_timer;
static struct k_work rssi_work;
static void rssi_timer_handler(struct k_timer *timer);
static void rssi_work_handler(struct k_work *work);

int ble_central_init(mipe_connection_cb_t conn_cb, mipe_rssi_cb_t rssi_cb)
{
    if (!conn_cb || !rssi_cb) {
        LOG_ERR("Invalid callbacks provided");
        return -EINVAL;
    }

    mipe_conn_callback = conn_cb;
    mipe_rssi_callback = rssi_cb;

    /* Initialize RSSI measurement timer and work */
    k_timer_init(&rssi_timer, rssi_timer_handler, NULL);
    k_work_init(&rssi_work, rssi_work_handler);

    LOG_INF("BLE Central TEST VERSION initialized");
    LOG_INF("Will generate simulated RSSI when triggered");
    return 0;
}

int ble_central_start_scan(void)
{
    /* In test mode, we don't actually scan */
    LOG_INF("TEST MODE: Simulating scan start (no actual scanning)");
    scanning = true;
    
    /* Start the RSSI timer to simulate connection */
    if (!test_mode_active) {
        test_mode_active = true;
        LOG_INF("TEST MODE: Starting simulated RSSI generation (1Hz)");
        k_timer_start(&rssi_timer, K_SECONDS(1), K_SECONDS(1));
    }
    
    return 0;
}

int ble_central_stop_scan(void)
{
    LOG_INF("TEST MODE: Simulating scan stop");
    scanning = false;
    
    /* Stop the RSSI timer */
    if (test_mode_active) {
        test_mode_active = false;
        k_timer_stop(&rssi_timer);
        LOG_INF("TEST MODE: Stopped simulated RSSI generation");
    }
    
    return 0;
}

int ble_central_disconnect_mipe(void)
{
    LOG_INF("TEST MODE: Simulating Mipe disconnect");
    
    /* Stop RSSI timer */
    if (test_mode_active) {
        test_mode_active = false;
        k_timer_stop(&rssi_timer);
    }
    
    /* Notify callback */
    if (mipe_conn_callback) {
        mipe_conn_callback(false);
    }
    
    return 0;
}

int ble_central_request_rssi(void)
{
    /* Trigger immediate RSSI measurement */
    k_work_submit(&rssi_work);
    return 0;
}

bool ble_central_is_connected(void)
{
    /* In test mode, we're "connected" when the timer is active */
    return test_mode_active;
}

/* RSSI measurement timer handler */
static void rssi_timer_handler(struct k_timer *timer)
{
    /* Submit work to measure RSSI (can't do BT operations in timer context) */
    k_work_submit(&rssi_work);
}

/* RSSI measurement work handler */
static void rssi_work_handler(struct k_work *work)
{
    /* Fixed RSSI value for testing */
    int8_t fixed_rssi = -55;
    uint32_t timestamp = k_uptime_get_32();
    
    LOG_DBG("TEST MODE: Generating fixed RSSI: %d dBm", fixed_rssi);
    
    /* Forward to callback */
    if (mipe_rssi_callback) {
        mipe_rssi_callback(fixed_rssi, timestamp);
    }
}
