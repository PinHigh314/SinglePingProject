/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Host Device Application
 * - Fixed RSSI test with alternating transmission
 * - Real Mipe connection with dual connection LED indication
 * - LED1 rapid flash (100ms) when both MotoApp and Mipe are connected
 * - Fixed LED1 timing issue to ensure solid ON when only MotoApp connected
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

#include "ble/ble_peripheral.h"
#include "ble/ble_central.h"

LOG_MODULE_REGISTER(host_main, LOG_LEVEL_INF);

/* LED definitions */
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);

/* Connection states */
static bool motoapp_connected = false;
static bool mipe_connected = false;
static bool streaming_active = false;

/* Timers */
static struct k_timer heartbeat_timer;
static struct k_timer led_flash_timer;
static struct k_timer led1_rapid_flash_timer;

/* RSSI alternation flag */
static bool use_fixed_rssi = true;

/* Latest RSSI from Mipe */
static int8_t latest_mipe_rssi = -70;
static uint32_t latest_mipe_timestamp = 0;

/* Timer handlers */
static void heartbeat_timer_handler(struct k_timer *timer)
{
    static bool led_state = false;
    gpio_pin_set_dt(&led0, led_state);
    led_state = !led_state;
}

static void led_flash_timer_handler(struct k_timer *timer)
{
    /* Turn off both RSSI indicator LEDs */
    gpio_pin_set_dt(&led2, 0);
    gpio_pin_set_dt(&led3, 0);
}

static void led1_rapid_flash_timer_handler(struct k_timer *timer)
{
    static bool led1_state = false;
    
    /* Only flash if both connections are active */
    if (motoapp_connected && mipe_connected) {
        led1_state = !led1_state;
        gpio_pin_set_dt(&led1, led1_state);
    } else {
        /* Safety: if timer is running but conditions changed, stop it */
        k_timer_stop(&led1_rapid_flash_timer);
        /* Set LED1 based on current state */
        if (motoapp_connected) {
            gpio_pin_set_dt(&led1, 1);  /* Solid ON */
        } else {
            gpio_pin_set_dt(&led1, 0);  /* OFF */
        }
    }
}

/* Helper function to update LED1 state */
static void update_led1_state(void)
{
    /* Always stop the timer first to prevent any race conditions */
    k_timer_stop(&led1_rapid_flash_timer);
    
    /* Small delay to ensure timer is fully stopped */
    k_sleep(K_MSEC(10));
    
    if (motoapp_connected && mipe_connected) {
        /* Both connected - start rapid flash */
        LOG_INF("LED1: Starting rapid flash (dual connection)");
        k_timer_start(&led1_rapid_flash_timer, K_NO_WAIT, K_MSEC(100));
    } else if (motoapp_connected) {
        /* Only MotoApp connected - solid ON */
        LOG_INF("LED1: Solid ON (MotoApp only)");
        gpio_pin_set_dt(&led1, 1);
    } else {
        /* No MotoApp connection - LED1 off */
        LOG_INF("LED1: OFF (no MotoApp)");
        gpio_pin_set_dt(&led1, 0);
    }
}

/* BLE Peripheral callbacks */
static void app_connected(void)
{
    LOG_INF("MotoApp connected");
    motoapp_connected = true;
    update_led1_state();
}

static void app_disconnected(void)
{
    LOG_INF("MotoApp disconnected");
    motoapp_connected = false;
    streaming_active = false;
    update_led1_state();
    gpio_pin_set_dt(&led3, 0);
}

static void streaming_state_changed(bool active)
{
    LOG_INF("Streaming %s", active ? "started" : "stopped");
    streaming_active = active;
    
    if (!active) {
        gpio_pin_set_dt(&led3, 0);
    }
}

/* BLE Central callbacks */
static void mipe_connection_changed(bool connected)
{
    LOG_INF("Mipe connection %s", connected ? "established" : "lost");
    mipe_connected = connected;
    update_led1_state();
}

static void mipe_rssi_received(int8_t rssi, uint32_t timestamp)
{
    LOG_DBG("Mipe RSSI: %d dBm at %u ms", rssi, timestamp);
    latest_mipe_rssi = rssi;
    latest_mipe_timestamp = timestamp;
}

/* Data transmission callback */
static int get_rssi_data(int8_t *rssi, uint32_t *timestamp)
{
    if (!streaming_active) {
        return -ENODATA;
    }
    
    int8_t rssi_to_send;
    
    /* Determine which RSSI to send */
    if (use_fixed_rssi) {
        /* Send fixed reference RSSI */
        rssi_to_send = -55;
        LOG_INF("TX Fixed RSSI: %d dBm (Reference)", rssi_to_send);
        
        /* Flash LED3 for fixed RSSI */
        gpio_pin_set_dt(&led3, 1);
        k_timer_start(&led_flash_timer, K_MSEC(200), K_NO_WAIT);
    } else {
        /* Send real RSSI */
        if (mipe_connected && latest_mipe_timestamp > 0) {
            /* Use actual Mipe RSSI */
            rssi_to_send = latest_mipe_rssi;
            LOG_INF("TX Real RSSI: %d dBm (from Mipe)", rssi_to_send);
        } else {
            /* Use simulated RSSI when Mipe not connected */
            rssi_to_send = -45 - (k_uptime_get_32() % 20);
            LOG_INF("TX Real RSSI: %d dBm (Simulated)", rssi_to_send);
        }
        
        /* Flash LED2 for real RSSI */
        gpio_pin_set_dt(&led2, 1);
        k_timer_start(&led_flash_timer, K_MSEC(200), K_NO_WAIT);
    }
    
    /* Toggle for next transmission */
    use_fixed_rssi = !use_fixed_rssi;
    
    *rssi = rssi_to_send;
    *timestamp = k_uptime_get_32();
    
    /* Keep LED3 on during streaming */
    if (streaming_active && !k_timer_remaining_get(&led_flash_timer)) {
        gpio_pin_set_dt(&led3, 1);
    }
    
    return 0;
}

static int init_leds(void)
{
    int ret;

    /* Check if devices are ready */
    if (!gpio_is_ready_dt(&led0)) {
        LOG_ERR("LED0 device not ready");
        return -ENODEV;
    }
    if (!gpio_is_ready_dt(&led1)) {
        LOG_ERR("LED1 device not ready");
        return -ENODEV;
    }
    if (!gpio_is_ready_dt(&led2)) {
        LOG_ERR("LED2 device not ready");
        return -ENODEV;
    }
    if (!gpio_is_ready_dt(&led3)) {
        LOG_ERR("LED3 device not ready");
        return -ENODEV;
    }

    /* Configure GPIOs */
    ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure LED0: %d", ret);
        return ret;
    }
    ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure LED1: %d", ret);
        return ret;
    }
    ret = gpio_pin_configure_dt(&led2, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure LED2: %d", ret);
        return ret;
    }
    ret = gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure LED3: %d", ret);
        return ret;
    }

    LOG_INF("All LEDs initialized successfully");
    return 0;
}

int main(void)
{
    int err;

    LOG_INF("=== Host Device Starting ===");
    LOG_INF("Features: Alternating RSSI + Real Mipe Connection");
    LOG_INF("LED1 timing fix for proper solid ON state");

    /* Initialize LEDs */
    err = init_leds();
    if (err) {
        LOG_ERR("LED initialization failed");
        return err;
    }

    /* Initialize timers */
    k_timer_init(&heartbeat_timer, heartbeat_timer_handler, NULL);
    k_timer_init(&led_flash_timer, led_flash_timer_handler, NULL);
    k_timer_init(&led1_rapid_flash_timer, led1_rapid_flash_timer_handler, NULL);

    /* Start heartbeat */
    k_timer_start(&heartbeat_timer, K_NO_WAIT, K_MSEC(500));
    LOG_INF("Heartbeat started (LED0)");

    /* Initialize Bluetooth */
    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return err;
    }
    LOG_INF("Bluetooth initialized");

    /* Initialize BLE Peripheral (for MotoApp connection) */
    err = ble_peripheral_init(app_connected, app_disconnected, 
                             streaming_state_changed, get_rssi_data);
    if (err) {
        LOG_ERR("Failed to initialize BLE peripheral: %d", err);
        return err;
    }

    /* Initialize BLE Central (for Mipe connection) */
    err = ble_central_init(mipe_connection_changed, mipe_rssi_received);
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
    LOG_INF("LED Status:");
    LOG_INF("  LED0: Heartbeat (blinking)");
    LOG_INF("  LED1: Solid=MotoApp only, Rapid flash=Dual connection");
    LOG_INF("  LED2: Real RSSI transmission flash");
    LOG_INF("  LED3: Fixed RSSI flash + Streaming active");

    /* Main loop */
    while (1) {
        k_sleep(K_SECONDS(1));
        
        /* Request RSSI from Mipe if connected */
        if (mipe_connected) {
            ble_central_request_rssi();
        }
    }

    return 0;
}
