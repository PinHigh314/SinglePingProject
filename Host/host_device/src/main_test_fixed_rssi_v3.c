/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * TEST VERSION v3: Enhanced debugging for RSSI transmission
 * This version adds detailed logging to diagnose why data isn't flowing
 * - Logs every step of the notification process
 * - Verifies GATT attribute configuration
 * - Checks notification state at multiple points
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
 * LED3: Data streaming activity (SOLID ON during streaming)
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

/* Data streaming timer for fixed RSSI */
static struct k_timer data_timer;
static void data_timer_handler(struct k_timer *timer);

/* Monitor timer to check streaming state */
static struct k_timer monitor_timer;
static void monitor_timer_handler(struct k_timer *timer);

/* Debug timer for periodic status */
static struct k_timer debug_timer;
static void debug_timer_handler(struct k_timer *timer);

/* Simulated RSSI data */
uint32_t packet_count = 0;  /* Made non-static for ble_peripheral.c access */
static int64_t stream_start_time = 0;
static int64_t last_packet_time = 0;

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

    LOG_INF("SinglePing Host Device TEST VERSION v3 - Enhanced Debug");
    LOG_INF("=======================================================");

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

    system_ready = true;
    
    /* Initialize and start heartbeat timer for LED0 (1000ms interval) */
    k_timer_init(&heartbeat_timer, heartbeat_timer_handler, NULL);
    k_timer_start(&heartbeat_timer, K_MSEC(1000), K_MSEC(1000));
    
    /* Initialize data streaming timer */
    k_timer_init(&data_timer, data_timer_handler, NULL);
    
    /* Initialize monitor timer to check streaming state */
    k_timer_init(&monitor_timer, monitor_timer_handler, NULL);
    k_timer_start(&monitor_timer, K_SECONDS(1), K_SECONDS(1));
    
    /* Initialize debug timer for periodic status reports */
    k_timer_init(&debug_timer, debug_timer_handler, NULL);
    k_timer_start(&debug_timer, K_SECONDS(5), K_SECONDS(5));
    
    LOG_INF("TEST MODE v3: Enhanced debugging enabled");
    LOG_INF("Will send fixed RSSI = -55 dBm when streaming starts");
    LOG_INF("LED0: Heartbeat, LED1: MotoApp, LED3: Streaming (solid)");
    LOG_INF("Debug status will be printed every 5 seconds");

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
        last_packet_time = 0;
        
        /* Log GATT service info */
        LOG_INF("GATT service ready for MotoApp");
        LOG_INF("Waiting for MotoApp to enable notifications...");
    } else {
        LOG_INF("MotoApp disconnected");
        gpio_pin_set_dt(&led1, 0); /* Clear connection indicator */
        data_streaming = false;
        gpio_pin_set_dt(&led3, 0); /* Clear streaming LED */
        k_timer_stop(&data_timer); /* Stop data timer */
    }
}

static void data_stream_callback(bool start)
{
    data_streaming = start;
    
    if (start) {
        stream_start_time = k_uptime_get();
        last_packet_time = k_uptime_get();
        packet_count = 0;
        LOG_INF("=== DATA STREAMING STARTED ===");
        LOG_INF("Command received from MotoApp to start streaming");
        LOG_INF("Fixed RSSI value: -55 dBm");
        LOG_INF("Transmission interval: 1 second");
        gpio_pin_set_dt(&led3, 1); /* Turn ON streaming LED (solid) */
        
        /* Start sending data immediately, then every 1 second */
        k_timer_start(&data_timer, K_NO_WAIT, K_SECONDS(1));
    } else {
        LOG_INF("=== DATA STREAMING STOPPED ===");
        LOG_INF("Total packets sent: %u", packet_count);
        gpio_pin_set_dt(&led3, 0); /* Turn OFF streaming LED */
        k_timer_stop(&data_timer);
    }
}

static void data_timer_handler(struct k_timer *timer)
{
    LOG_INF(">>> Data timer fired - attempting to send RSSI <<<");
    
    if (!motoapp_connected) {
        LOG_WRN("Cannot send - MotoApp not connected");
        return;
    }
    
    if (!data_streaming) {
        LOG_WRN("Cannot send - streaming not active");
        return;
    }
    
    if (!ble_peripheral_is_connected()) {
        LOG_WRN("Cannot send - BLE peripheral reports not connected");
        return;
    }
    
    if (!ble_peripheral_is_streaming()) {
        LOG_WRN("Cannot send - notifications not enabled");
        return;
    }
    
    /* Fixed RSSI value for testing */
    int8_t fixed_rssi = -55;
    uint32_t timestamp = k_uptime_get_32();
    
    LOG_INF("Sending RSSI data:");
    LOG_INF("  - RSSI: %d dBm", fixed_rssi);
    LOG_INF("  - Timestamp: %u ms", timestamp);
    LOG_INF("  - Packet #: %u", packet_count + 1);
    
    int err = ble_peripheral_send_rssi_data(fixed_rssi, timestamp);
    
    if (err == 0) {
        packet_count++;
        last_packet_time = k_uptime_get();
        LOG_INF("✓ SUCCESS: RSSI data sent (total: %u packets)", packet_count);
    } else {
        LOG_ERR("✗ FAILED to send RSSI data: %d", err);
        switch (err) {
            case -ENOTCONN:
                LOG_ERR("  Error: Not connected");
                break;
            case -EACCES:
                LOG_ERR("  Error: Notifications not enabled");
                break;
            case -ENOMEM:
                LOG_ERR("  Error: Buffer full");
                break;
            default:
                LOG_ERR("  Error: Unknown error %d", err);
                break;
        }
    }
    
    LOG_INF("<<< Data timer handler complete >>>");
}

static void monitor_timer_handler(struct k_timer *timer)
{
    /* Check if we should turn off LED3 based on streaming state */
    if (!data_streaming && gpio_pin_get_dt(&led3)) {
        LOG_INF("Monitor: Turning off LED3 - streaming stopped");
        gpio_pin_set_dt(&led3, 0);
    }
    
    /* Also check if notifications are disabled */
    if (!ble_peripheral_is_streaming() && gpio_pin_get_dt(&led3)) {
        LOG_INF("Monitor: Turning off LED3 - notifications disabled");
        gpio_pin_set_dt(&led3, 0);
        data_streaming = false;
        k_timer_stop(&data_timer);
    }
}

static void debug_timer_handler(struct k_timer *timer)
{
    LOG_INF("=== SYSTEM STATUS ===");
    LOG_INF("MotoApp connected: %s", motoapp_connected ? "YES" : "NO");
    LOG_INF("BLE peripheral connected: %s", ble_peripheral_is_connected() ? "YES" : "NO");
    LOG_INF("Notifications enabled: %s", ble_peripheral_is_streaming() ? "YES" : "NO");
    LOG_INF("Data streaming: %s", data_streaming ? "YES" : "NO");
    LOG_INF("Packets sent: %u", packet_count);
    
    if (data_streaming && packet_count == 0) {
        LOG_WRN("!!! WARNING: Streaming active but no packets sent !!!");
        LOG_WRN("Check if MotoApp has enabled notifications on RSSI characteristic");
    }
    
    LOG_INF("LED states: LED0=%d, LED1=%d, LED2=%d, LED3=%d",
            gpio_pin_get_dt(&led0), gpio_pin_get_dt(&led1),
            gpio_pin_get_dt(&led2), gpio_pin_get_dt(&led3));
    LOG_INF("===================");
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
    /* Not used in test mode - we send fixed data instead */
    LOG_DBG("Ignoring Mipe RSSI in test mode");
}

static void heartbeat_timer_handler(struct k_timer *timer)
{
    /* Toggle LED0 for heartbeat effect */
    heartbeat_state = !heartbeat_state;
    gpio_pin_set_dt(&led0, heartbeat_state ? 1 : 0);
}
