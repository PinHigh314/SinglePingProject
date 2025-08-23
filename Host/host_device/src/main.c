/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * SinglePing Host Device - Central Measurement Controller
 * 
 * SYSTEM ARCHITECTURE:
 * MotoApp (Android) ←→ BLE ←→ Host (NRF54L15DK) ←→ BLE ←→ Mipe Device
 * 
 * HOST DEVICE ROLE:
 * - Central measurement controller and BLE coordinator
 * - Sequential BLE communication (MotoApp OR Mipe, never simultaneous)
 * - RSSI sampling with fixed RF parameters for 10% accuracy target
 * - Distance calculation algorithms
 * - Real-time data streaming to MotoApp
 * 
 * POWER:
 * - USB-C from phone (power-only, no data communication)
 * - No battery constraints for Host device
 * 
 * TMT PHASES:
 * - TMT1: UI Foundation (mock data)
 * - TMT2: Host-MotoApp BLE integration (current phase)
 * - TMT3: Mipe device integration
 * - TMT4: Real distance calculation
 * - TMT5: RF optimization
 * - TMT6: Power optimization (Mipe 30+ day battery target)
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
 * LED2: Mipe connection status (TMT3+)
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
static bool mipe_connected = false;
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

    LOG_INF("SinglePing Host Device starting...");

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

    /* Initialize BLE as Central for Mipe device connections */
    err = ble_central_init(mipe_connection_callback, mipe_rssi_callback);
    if (err) {
        LOG_ERR("BLE Central initialization failed: %d", err);
        return err;
    }

    /* NOTE: Scanning for Mipe devices will be started only after MotoApp connects
     * to prevent interference with advertising. This ensures MotoApp can always
     * discover and connect to the Host device.
     */

    system_ready = true;
    
    /* Initialize and start heartbeat timer for LED0 (1000ms interval) */
    k_timer_init(&heartbeat_timer, heartbeat_timer_handler, NULL);
    k_timer_start(&heartbeat_timer, K_MSEC(1000), K_MSEC(1000));
    
    LOG_INF("TMT3 Host Device initialized - Dual-role BLE enabled");
    LOG_INF("Waiting for MotoApp connection (Peripheral mode active)");
    LOG_INF("LED0: Heartbeat (1000ms), LED1: MotoApp status, LED2: Mipe status");
    LOG_INF("Mipe scanning will start after MotoApp connects");

    /* Main loop */
    while (1) {
        /* Data will only flow when BOTH connections are active */
        /* Real RSSI from Mipe is forwarded via mipe_rssi_callback */
        
        /* Sleep for 100ms (10Hz update rate) */
        k_msleep(100);
    }

    return 0;
}

static void system_init(void)
{
    LOG_INF("Initializing system...");
    
    /* System initialization - caches are handled by Zephyr */
    
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
    int err;
    motoapp_connected = connected;
    
    if (connected) {
        LOG_INF("MotoApp connected via BLE");
        gpio_pin_set_dt(&led1, 1); /* Connection indicator */
        packet_count = 0;
        
        /* Now that MotoApp is connected, start scanning for Mipe devices
         * This prevents scanning from interfering with advertising
         */
        LOG_INF("Starting Mipe device scanning now that MotoApp is connected");
        err = ble_central_start_scan();
        if (err) {
            LOG_WRN("Failed to start Mipe scanning: %d", err);
            /* Continue anyway - can retry later */
        } else {
            LOG_INF("Mipe scanning started successfully");
        }
    } else {
        LOG_INF("MotoApp disconnected");
        gpio_pin_set_dt(&led1, 0); /* Clear connection indicator */
        data_streaming = false;
        gpio_pin_set_dt(&led3, 0); /* Clear streaming LED if it was on */
        
        /* Stop scanning for Mipe devices when MotoApp disconnects
         * This ensures advertising can resume without interference
         */
        LOG_INF("Stopping Mipe device scanning to allow advertising");
        err = ble_central_stop_scan();
        if (err) {
            LOG_WRN("Failed to stop Mipe scanning: %d", err);
        }
    }
}

static void data_stream_callback(bool start)
{
    data_streaming = start;
    
    if (start) {
        stream_start_time = k_uptime_get();
        packet_count = 0;
        LOG_INF("Data streaming started by MotoApp");
    } else {
        LOG_INF("Data streaming stopped by MotoApp");
        gpio_pin_set_dt(&led3, 0); /* Turn off streaming LED */
    }
}

static void mipe_connection_callback(bool connected)
{
    mipe_connected = connected;
    
    if (connected) {
        LOG_INF("Mipe device connected via BLE Central");
        gpio_pin_set_dt(&led2, 1); /* Mipe connection indicator */
    } else {
        LOG_INF("Mipe device disconnected");
        gpio_pin_set_dt(&led2, 0); /* Clear Mipe connection indicator */
    }
}

static void mipe_rssi_callback(int8_t rssi, uint32_t timestamp)
{
    static uint32_t last_send_time = 0;
    uint32_t current_time = k_uptime_get_32();
    
    /* Rate limit: Only send once every 2 seconds for stability */
    if (current_time - last_send_time < 2000) {
        LOG_DBG("Rate limiting RSSI data (waiting for 2s interval)");
        return;
    }
    
    /* Forward real RSSI from Mipe to MotoApp */
    if (motoapp_connected && data_streaming && ble_peripheral_is_connected()) {
        /* Add small delay to ensure BLE stack stability */
        k_msleep(50);
        
        int err = ble_peripheral_send_rssi_data(rssi, timestamp);
        
        if (err == 0) {
            packet_count++;
            last_send_time = current_time;
            
            /* Flash LED3 to show data streaming activity */
            if (packet_count % 5 == 0) { /* Flash every 5 packets (10 seconds) */
                gpio_pin_toggle_dt(&led3);
            }
            
            LOG_INF("Forwarded Mipe RSSI: %d dBm (packet %u) - Next in 2s", rssi, packet_count);
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
