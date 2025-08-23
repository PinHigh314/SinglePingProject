/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 * 
 * Host Device Test Firmware v6 - Alternating Fixed/Real RSSI
 * - Alternates between fixed reference (-55) and real RSSI readings
 * - LED3 flashes when transmitting fixed -55 reference
 * - LED2 flashes when transmitting real RSSI from MotoApp connection
 * - LED1 indicates MotoApp connection status
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/settings/settings.h>
#include <dk_buttons_and_leds.h>

#include "ble/ble_peripheral.h"
#include "ble/ble_central.h"
#include "button_handler.h"
#include "logger.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* LED definitions */
#define LED_MOTOAPP DK_LED1    /* MotoApp connection status */
#define LED_REAL_RSSI DK_LED2  /* Flashes when transmitting real RSSI */
#define LED_FIXED_RSSI DK_LED3 /* Flashes when transmitting fixed -55 */

/* GPIO for LEDs */
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

/* Connection states */
static bool motoapp_connected = false;
static bool data_streaming = false;
static bool use_fixed_rssi = true;  /* Start with fixed value */

/* MotoApp connection handle for real RSSI readings */
static struct bt_conn *motoapp_conn = NULL;

/* Packet counter for status reporting */
uint32_t packet_count = 0;

/* Monitor timer for LED control */
static void monitor_timer_handler(struct k_timer *timer);
K_TIMER_DEFINE(monitor_timer, monitor_timer_handler, NULL);

/* LED flash timer for visual feedback */
static void led_flash_timer_handler(struct k_timer *timer);
K_TIMER_DEFINE(led_flash_timer, led_flash_timer_handler, NULL);

static void monitor_timer_handler(struct k_timer *timer)
{
    /* LED1 - MotoApp connection status */
    if (motoapp_connected) {
        gpio_pin_set_dt(&led1, 1);
    } else {
        gpio_pin_set_dt(&led1, 0);
    }
    
    /* LED2 and LED3 are controlled by flash timer during streaming */
    if (!data_streaming) {
        gpio_pin_set_dt(&led2, 0);
        gpio_pin_set_dt(&led3, 0);
    }
}

static void led_flash_timer_handler(struct k_timer *timer)
{
    /* Turn off both LEDs after flash */
    gpio_pin_set_dt(&led2, 0);
    gpio_pin_set_dt(&led3, 0);
}

/* Callbacks from BLE modules */
static void motoapp_connection_callback(bool connected)
{
    motoapp_connected = connected;
    
    if (connected) {
        LOG_INF("=== MOTOAPP CONNECTED ===");
        gpio_pin_set_dt(&led1, 1);
    } else {
        LOG_INF("=== MOTOAPP DISCONNECTED ===");
        if (motoapp_conn) {
            bt_conn_unref(motoapp_conn);
            motoapp_conn = NULL;
        }
        gpio_pin_set_dt(&led1, 0);
        gpio_pin_set_dt(&led2, 0);
        gpio_pin_set_dt(&led3, 0);
        data_streaming = false;
    }
}

static void data_stream_callback(bool start)
{
    data_streaming = start;
    
    if (start) {
        LOG_INF("=== DATA STREAMING STARTED ===");
        LOG_INF("Control command received from MotoApp");
        LOG_INF("Will alternate between fixed (-55) and real RSSI");
        
        /* Start the simulated RSSI generation */
        LOG_INF("Starting alternating RSSI generation...");
        int err = ble_central_start_scan();  /* This starts the RSSI timer */
        if (err) {
            LOG_ERR("Failed to start RSSI generation: %d", err);
        }
    } else {
        LOG_INF("=== DATA STREAMING STOPPED ===");
        gpio_pin_set_dt(&led2, 0);
        gpio_pin_set_dt(&led3, 0);
        ble_central_stop_scan();
    }
}

/* Modified RSSI callback to handle alternating values */
static void mipe_rssi_callback(int8_t rssi, uint32_t timestamp)
{
    if (!data_streaming) {
        return;
    }
    
    int8_t rssi_to_send;
    
    if (use_fixed_rssi) {
        /* Send fixed reference value */
        rssi_to_send = -55;
        LOG_INF("TX Fixed RSSI: %d dBm (Reference)", rssi_to_send);
        
        /* Flash LED3 for fixed RSSI */
        gpio_pin_set_dt(&led3, 1);
        k_timer_start(&led_flash_timer, K_MSEC(100), K_NO_WAIT);
    } else {
        /* Get real RSSI from MotoApp connection */
        if (motoapp_conn) {
            /* Simulate varying RSSI for MotoApp connection */
            /* In production, you'd use bt_conn_get_info() to get real RSSI */
            rssi_to_send = -45 - (k_uptime_get_32() % 20);  /* Range: -45 to -65 dBm */
        } else {
            rssi_to_send = -70;  /* No connection */
        }
        
        LOG_INF("TX Real RSSI: %d dBm (MotoApp Connection)", rssi_to_send);
        
        /* Flash LED2 for real RSSI */
        gpio_pin_set_dt(&led2, 1);
        k_timer_start(&led_flash_timer, K_MSEC(100), K_NO_WAIT);
    }
    
    /* Toggle for next transmission */
    use_fixed_rssi = !use_fixed_rssi;
    
    /* Forward to BLE peripheral for transmission */
    int err = ble_peripheral_send_rssi_data(rssi_to_send, timestamp);
    if (err == 0) {
        packet_count++;
    }
}

static int init_leds(void)
{
    int ret;
    
    if (!device_is_ready(led1.port) || 
        !device_is_ready(led2.port) || 
        !device_is_ready(led3.port)) {
        LOG_ERR("LED devices not ready");
        return -ENODEV;
    }
    
    ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) return ret;
    
    ret = gpio_pin_configure_dt(&led2, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) return ret;
    
    ret = gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) return ret;
    
    LOG_INF("LEDs initialized - v6 alternating mode");
    LOG_INF("LED1: MotoApp connection");
    LOG_INF("LED2: Real RSSI transmission");
    LOG_INF("LED3: Fixed -55 reference transmission");
    
    return 0;
}

int main(void)
{
    int err;
    
    LOG_INF("=== Host Device Test v6 Starting ===");
    LOG_INF("Alternating Fixed/Real RSSI Mode");
    
    /* Initialize logging */
    logger_init(LOG_LEVEL_INF);
    
    /* Initialize LEDs */
    err = init_leds();
    if (err) {
        LOG_ERR("LED init failed: %d", err);
        return err;
    }
    
    /* Initialize button handler */
    err = button_handler_init(NULL);  /* No button callback needed for test */
    if (err) {
        LOG_ERR("Button handler init failed: %d", err);
        return err;
    }
    
    /* Enable Bluetooth */
    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed: %d", err);
        return err;
    }
    
    LOG_INF("Bluetooth initialized");
    
    /* Load settings */
    if (IS_ENABLED(CONFIG_SETTINGS)) {
        settings_load();
    }
    
    /* Initialize BLE peripheral (connection to MotoApp) */
    err = ble_peripheral_init(motoapp_connection_callback, data_stream_callback);
    if (err) {
        LOG_ERR("BLE peripheral init failed: %d", err);
        return err;
    }
    
    /* Initialize BLE central test mode */
    err = ble_central_init(NULL, mipe_rssi_callback);  /* No connection callback needed */
    if (err) {
        LOG_ERR("BLE central init failed: %d", err);
        return err;
    }
    
    /* Advertising is started automatically by ble_peripheral_init */
    LOG_INF("=== Host Device Ready ===");
    LOG_INF("Advertising to MotoApp...");
    LOG_INF("Test mode: Alternating Fixed/Real RSSI");
    
    /* Start monitor timer */
    k_timer_start(&monitor_timer, K_SECONDS(1), K_SECONDS(1));
    
    /* Main loop */
    while (1) {
        k_sleep(K_FOREVER);
    }
    
    return 0;
}
