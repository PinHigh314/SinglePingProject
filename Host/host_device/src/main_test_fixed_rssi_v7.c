/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 * 
 * Host Device Test Firmware v7 - Fixed Alternating RSSI
 * - Based on working v5 structure with v6 alternating feature
 * - LED0: Heartbeat (system alive)
 * - LED1: MotoApp connection status
 * - LED2: Flashes for real RSSI transmission
 * - LED3: Flashes for fixed -55 reference transmission
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/random/random.h>

#include "ble/ble_peripheral.h"
#include "ble/ble_central.h"
#include "button_handler.h"
#include "logger.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* LED definitions - CORRECT mapping */
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
static bool data_streaming = false;
static bool use_fixed_rssi = true;  /* Start with fixed value */

/* LED heartbeat timer */
static struct k_timer heartbeat_timer;
static bool heartbeat_state = false;

/* LED flash timer for RSSI indicators */
static struct k_timer led_flash_timer;

/* Packet counter */
uint32_t packet_count = 0;

/* Forward declarations */
static void system_init(void);
static void led_init(void);
static void motoapp_connection_callback(bool connected);
static void mipe_connection_callback(bool connected);
static void mipe_rssi_callback(int8_t rssi, uint32_t timestamp);
static void data_stream_callback(bool start);
static void heartbeat_timer_handler(struct k_timer *timer);
static void led_flash_timer_handler(struct k_timer *timer);

int main(void)
{
    int err;

    LOG_INF("SinglePing Host Device TEST VERSION v7 - Fixed Alternating RSSI");
    LOG_INF("================================================================");

    /* Initialize system components */
    system_init();

    /* Initialize LEDs */
    led_init();

    /* Initialize logger */
    logger_init(LOGGER_LEVEL_INFO);

    /* Initialize button handler */
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

    /* Initialize BLE as Central with callbacks */
    err = ble_central_init(mipe_connection_callback, mipe_rssi_callback);
    if (err) {
        LOG_ERR("BLE Central initialization failed: %d", err);
        return err;
    }

    system_ready = true;
    
    /* Initialize and start heartbeat timer for LED0 */
    k_timer_init(&heartbeat_timer, heartbeat_timer_handler, NULL);
    k_timer_start(&heartbeat_timer, K_MSEC(1000), K_MSEC(1000));
    
    /* Initialize LED flash timer */
    k_timer_init(&led_flash_timer, led_flash_timer_handler, NULL);
    
    LOG_INF("TEST MODE v7: Alternating Fixed/Real RSSI");
    LOG_INF("LED0: Heartbeat, LED1: MotoApp connection");
    LOG_INF("LED2: Real RSSI flash, LED3: Fixed -55 flash");
    LOG_INF("Streaming starts when MotoApp sends control command");

    /* Main loop */
    while (1) {
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

    /* Configure LED0 - Heartbeat */
    if (!gpio_is_ready_dt(&led0)) {
        LOG_ERR("LED0 device not ready");
        return;
    }
    ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Cannot configure LED0: %d", ret);
        return;
    }

    /* Configure LED1 - MotoApp connection */
    if (!gpio_is_ready_dt(&led1)) {
        LOG_ERR("LED1 device not ready");
        return;
    }
    ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Cannot configure LED1: %d", ret);
        return;
    }

    /* Configure LED2 - Real RSSI indicator */
    if (!gpio_is_ready_dt(&led2)) {
        LOG_ERR("LED2 device not ready");
        return;
    }
    ret = gpio_pin_configure_dt(&led2, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Cannot configure LED2: %d", ret);
        return;
    }

    /* Configure LED3 - Fixed RSSI indicator */
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
        gpio_pin_set_dt(&led1, 1);
        packet_count = 0;
        use_fixed_rssi = true;  /* Reset to start with fixed */
        LOG_INF("Waiting for streaming command from MotoApp...");
    } else {
        LOG_INF("MotoApp disconnected");
        gpio_pin_set_dt(&led1, 0);
        gpio_pin_set_dt(&led2, 0);
        gpio_pin_set_dt(&led3, 0);
        data_streaming = false;
        ble_central_stop_scan();
    }
}

static void data_stream_callback(bool start)
{
    data_streaming = start;
    
    if (start) {
        packet_count = 0;
        use_fixed_rssi = true;  /* Start with fixed value */
        LOG_INF("=== DATA STREAMING STARTED ===");
        LOG_INF("Will alternate between fixed (-55) and real RSSI");
        
        /* Start the simulated RSSI generation */
        int err = ble_central_start_scan();
        if (err) {
            LOG_ERR("Failed to start RSSI generation: %d", err);
        } else {
            LOG_INF("RSSI generation started successfully");
        }
    } else {
        LOG_INF("=== DATA STREAMING STOPPED ===");
        LOG_INF("Total packets sent: %u", packet_count);
        gpio_pin_set_dt(&led2, 0);
        gpio_pin_set_dt(&led3, 0);
        ble_central_stop_scan();
    }
}

static void mipe_connection_callback(bool connected)
{
    /* Not used in test mode */
    LOG_DBG("TEST MODE: Mipe connection callback (ignored)");
}

static void mipe_rssi_callback(int8_t rssi, uint32_t timestamp)
{
    static uint32_t last_send_time = 0;
    uint32_t current_time = k_uptime_get_32();
    int8_t rssi_to_send;
    
    /* Rate limit: 2 seconds between transmissions */
    if (current_time - last_send_time < 2000) {
        return;
    }
    
    if (!motoapp_connected || !data_streaming) {
        return;
    }
    
    /* Determine which RSSI to send */
    if (use_fixed_rssi) {
        rssi_to_send = -55;
        LOG_INF("TX Fixed RSSI: %d dBm (Reference)", rssi_to_send);
        
        /* Flash LED3 for fixed RSSI */
        gpio_pin_set_dt(&led3, 1);
        k_timer_start(&led_flash_timer, K_MSEC(200), K_NO_WAIT);
    } else {
        /* Simulate varying RSSI */
        rssi_to_send = -45 - (k_uptime_get_32() % 20);
        LOG_INF("TX Real RSSI: %d dBm (Simulated)", rssi_to_send);
        
        /* Flash LED2 for real RSSI */
        gpio_pin_set_dt(&led2, 1);
        k_timer_start(&led_flash_timer, K_MSEC(200), K_NO_WAIT);
    }
    
    /* Toggle for next transmission */
    use_fixed_rssi = !use_fixed_rssi;
    
    /* Add small delay for BLE stability */
    k_msleep(50);
    
    /* Send to MotoApp */
    int err = ble_peripheral_send_rssi_data(rssi_to_send, timestamp);
    if (err == 0) {
        packet_count++;
        last_send_time = current_time;
        LOG_INF("Sent packet %u - Next in 2s", packet_count);
    } else {
        LOG_WRN("Failed to send RSSI: %d", err);
    }
}

static void heartbeat_timer_handler(struct k_timer *timer)
{
    /* Toggle LED0 for heartbeat */
    heartbeat_state = !heartbeat_state;
    gpio_pin_set_dt(&led0, heartbeat_state ? 1 : 0);
}

static void led_flash_timer_handler(struct k_timer *timer)
{
    /* Turn off both indicator LEDs */
    gpio_pin_set_dt(&led2, 0);
    gpio_pin_set_dt(&led3, 0);
}
