/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * TEST VERSION v5: Fixed LED2 issue and streaming trigger
 * - LED2 no longer lights up on MotoApp connection
 * - Streaming properly triggered by control command
 * - Uses callback architecture like rev005
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/random/random.h>
#include <math.h>

#include "ble/ble_peripheral.h"
#include "ble/ble_central.h"
#include "button_handler.h"
#include "logger.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/**
 * LED Status Indicators:
 * LED0: Heartbeat/System ready
 * LED1: MotoApp connection status
 * LED2: Reserved for Mipe (NOT USED in test)
 * LED3: Data streaming activity
 */
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);

/* System state */
static bool system_ready = false;
static bool motoapp_connected = false;
static bool mipe_connected = false;  /* Always false in test mode */
static bool data_streaming = false;

/* LED heartbeat timer */
static struct k_timer heartbeat_timer;
static bool heartbeat_state = false;

/* Simulated RSSI data */
uint32_t packet_count = 0;  /* Made non-static for ble_peripheral.c access */
static int64_t stream_start_time = 0;

/* Forward declarations */
static void system_init(void);
static void led_init(void);
static void motoapp_connection_callback(bool connected);
static void mipe_connection_callback(bool connected);
static void mipe_rssi_callback(int8_t rssi, uint32_t timestamp);
static void data_stream_callback(bool start);
static void heartbeat_timer_handler(struct k_timer *timer);

int main(void)
{
    int err;

    LOG_INF("SinglePing Host Device TEST VERSION v5 - Fixed Streaming");
    LOG_INF("=========================================================");

    /* Initialize system components */
    system_init();

    /* Initialize LEDs */
    led_init();

    /* Initialize logger */
    logger_init(LOGGER_LEVEL_INFO);

    /* Initialize button handler for data stream control */
    err = button_handler_init(NULL);
    if (err) {
        LOG_ERR("Button handler initialization failed: %d", err);
        return err;
    }

    /* Initialize BLE as Peripheral for MotoApp connection */
    err = ble_peripheral_init(motoapp_connection_callback, data_stream_callback);
    if (err) {
        LOG_ERR("BLE Peripheral initialization failed: %d", err);
        return err;
    }

    /* Initialize BLE as Central with our callbacks
     * In test mode, this will generate simulated RSSI data
     */
    err = ble_central_init(mipe_connection_callback, mipe_rssi_callback);
    if (err) {
        LOG_ERR("BLE Central initialization failed: %d", err);
        return err;
    }

    system_ready = true;
    
    /* Initialize and start heartbeat timer for LED0 (1000ms interval) */
    k_timer_init(&heartbeat_timer, heartbeat_timer_handler, NULL);
    k_timer_start(&heartbeat_timer, K_MSEC(1000), K_MSEC(1000));
    
    LOG_INF("TEST MODE v5: Fixed LED2 and streaming trigger");
    LOG_INF("LED0: Heartbeat, LED1: MotoApp, LED3: Streaming");
    LOG_INF("LED2: OFF (reserved for future Mipe connection)");
    LOG_INF("Streaming starts when MotoApp sends control command");

    /* Main loop */
    while (1) {
        /* Sleep for 100ms */
        k_msleep(100);
    }

    return 0;
}

static void system_init(void)
{
    LOG_INF("Initializing system...");
    LOG_INF("System initialization complete");
}

static void led_init(void)
{
    int ret;

    /* Configure LEDs */
    if (!gpio_is_ready_dt(&led0)) {
        LOG_ERR("LED0 device not ready");
        return;
    }
    ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Cannot configure LED0: %d", ret);
        return;
    }

    if (!gpio_is_ready_dt(&led1)) {
        LOG_ERR("LED1 device not ready");
        return;
    }
    ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Cannot configure LED1: %d", ret);
        return;
    }

    if (!gpio_is_ready_dt(&led2)) {
        LOG_ERR("LED2 device not ready");
        return;
    }
    ret = gpio_pin_configure_dt(&led2, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Cannot configure LED2: %d", ret);
        return;
    }

    if (!gpio_is_ready_dt(&led3)) {
        LOG_ERR("LED3 device not ready");
        return;
    }
    ret = gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Cannot configure LED3: %d", ret);
        return;
    }

    LOG_INF("LEDs initialized");
}

static void motoapp_connection_callback(bool connected)
{
    motoapp_connected = connected;
    
    if (connected) {
        LOG_INF("MotoApp connected via BLE");
        gpio_pin_set_dt(&led1, 1); /* Connection indicator */
        packet_count = 0;
        
        /* DO NOT simulate Mipe connection here - wait for streaming command */
        LOG_INF("Waiting for streaming command from MotoApp...");
    } else {
        LOG_INF("MotoApp disconnected");
        gpio_pin_set_dt(&led1, 0); /* Clear connection indicator */
        data_streaming = false;
        gpio_pin_set_dt(&led3, 0); /* Clear streaming LED */
        
        /* Stop simulated RSSI generation if it was running */
        ble_central_stop_scan();
    }
}

static void data_stream_callback(bool start)
{
    data_streaming = start;
    
    if (start) {
        stream_start_time = k_uptime_get();
        packet_count = 0;
        LOG_INF("=== DATA STREAMING STARTED ===");
        LOG_INF("Control command received from MotoApp");
        gpio_pin_set_dt(&led3, 1); /* Turn ON streaming LED */
        
        /* NOW start the simulated RSSI generation */
        LOG_INF("Starting simulated RSSI generation...");
        int err = ble_central_start_scan();  /* This starts the RSSI timer */
        if (err) {
            LOG_ERR("Failed to start RSSI generation: %d", err);
        } else {
            LOG_INF("RSSI generation started successfully");
        }
    } else {
        LOG_INF("=== DATA STREAMING STOPPED ===");
        LOG_INF("Total packets sent: %u", packet_count);
        gpio_pin_set_dt(&led3, 0); /* Turn OFF streaming LED */
        
        /* Stop simulated RSSI generation */
        ble_central_stop_scan();
    }
}

static void mipe_connection_callback(bool connected)
{
    /* In test mode, we don't use this callback */
    LOG_DBG("TEST MODE: Mipe connection callback (ignored): %s", 
            connected ? "connected" : "disconnected");
}

static void mipe_rssi_callback(int8_t rssi, uint32_t timestamp)
{
    static uint32_t last_send_time = 0;
    uint32_t current_time = k_uptime_get_32();
    
    LOG_DBG("TEST MODE: Received simulated RSSI callback: %d dBm", rssi);
    
    /* Rate limit: Only send once every 2 seconds for stability (like rev005) */
    if (current_time - last_send_time < 2000) {
        LOG_DBG("Rate limiting RSSI data (waiting for 2s interval)");
        return;
    }
    
    /* Forward RSSI to MotoApp */
    if (motoapp_connected && data_streaming && ble_peripheral_is_connected()) {
        /* Add small delay to ensure BLE stack stability */
        k_msleep(50);
        
        int err = ble_peripheral_send_rssi_data(rssi, timestamp);
        
        if (err == 0) {
            packet_count++;
            last_send_time = current_time;
            
            /* Toggle LED3 to show data streaming activity */
            gpio_pin_toggle_dt(&led3);
            
            LOG_INF("Forwarded RSSI: %d dBm (packet %u) - Next in 2s", 
                    rssi, packet_count);
        } else {
            LOG_WRN("Failed to forward RSSI data: %d (will retry in 2s)", err);
            /* On error, don't update last_send_time to retry sooner */
        }
    } else {
        LOG_DBG("Not forwarding RSSI - MotoApp:%d Stream:%d Connected:%d", 
                motoapp_connected, data_streaming, ble_peripheral_is_connected());
    }
}

static void heartbeat_timer_handler(struct k_timer *timer)
{
    /* Toggle LED0 for heartbeat effect */
    heartbeat_state = !heartbeat_state;
    gpio_pin_set_dt(&led0, heartbeat_state ? 1 : 0);
}
