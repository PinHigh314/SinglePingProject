/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Host Device Test Application - v9
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

LOG_MODULE_REGISTER(host_main_v9, LOG_LEVEL_INF);

/* BLE logging buffer */
static char ble_log_buffer[128];

/* Helper function to send logs via BLE */
static void log_ble(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(ble_log_buffer, sizeof(ble_log_buffer), format, args);
    va_end(args);

    LOG_INF("%s", ble_log_buffer);
    ble_peripheral_send_log_data(ble_log_buffer);
}

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

/* Latest RSSI from Mipe */
static int8_t latest_mipe_rssi = -70;
static uint32_t latest_mipe_timestamp = 0;

/* Mipe status data */
static mipe_status_t mipe_status = {0};
static uint16_t mipe_connection_attempts = 0;

/* Timer handlers */
static void heartbeat_timer_handler(struct k_timer *timer)
{
    static bool led_state = false;
    gpio_pin_set_dt(&led0, led_state);
    led_state = !led_state;
}

static void led_flash_timer_handler(struct k_timer *timer)
{
    /* Turn off RSSI indicator LED */
    gpio_pin_set_dt(&led2, 0);
}

static void led1_rapid_flash_timer_handler(struct k_timer *timer)
{
    static bool led1_state = false;
    static int log_counter = 0;
    
    /* Log every 10th call to avoid log spam */
    if (++log_counter >= 10) {
        log_counter = 0;
        log_ble("LED1 Timer: MotoApp=%s, Mipe=%s", 
                motoapp_connected ? "YES" : "NO",
                mipe_connected ? "YES" : "NO");
    }
    
    /* Only flash if both connections are active */
    if (motoapp_connected && mipe_connected) {
        led1_state = !led1_state;
        gpio_pin_set_dt(&led1, led1_state);
    } else {
    /* Safety: if timer is running but conditions changed, stop it */
        log_ble("LED1 Timer: Stopping timer - conditions changed!");
        k_timer_stop(&led1_rapid_flash_timer);
        /* Set LED1 based on current state */
        if (motoapp_connected && !mipe_connected) {
            log_ble("LED1 Timer: Setting to solid ON (MotoApp only)");
            gpio_pin_set_dt(&led1, 1);  /* Solid ON */
        } else {
            log_ble("LED1 Timer: Setting to OFF");
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
    
    log_ble("=== LED1 State Update ===");
    log_ble("  MotoApp connected: %s", motoapp_connected ? "YES" : "NO");
    log_ble("  Mipe connected: %s", mipe_connected ? "YES" : "NO");
    log_ble("  Timer running: %s", k_timer_remaining_get(&led1_rapid_flash_timer) > 0 ? "YES" : "NO");
    
    if (motoapp_connected && mipe_connected) {
        /* Both connected - start rapid flash */
        log_ble("LED1: Starting rapid flash (dual connection)");
        gpio_pin_set_dt(&led1, 1); /* Ensure LED is on before starting timer */
        k_timer_start(&led1_rapid_flash_timer, K_NO_WAIT, K_MSEC(100));
    } else if (motoapp_connected && !mipe_connected) {
        /* Only MotoApp connected - solid ON */
        log_ble("LED1: Setting to Solid ON (MotoApp only, Mipe NOT connected)");
        gpio_pin_set_dt(&led1, 1);
    } else {
        /* No MotoApp connection - LED1 off */
        log_ble("LED1: Setting to OFF (no MotoApp)");
        gpio_pin_set_dt(&led1, 0);
    }
    log_ble("=== LED1 State Update Complete ===");
}

static void update_mipe_status(void)
{
    mipe_status.status_flags = (ble_central_is_scanning() ? 1 : 0) | (mipe_connected ? 2 : 0);
    mipe_status.rssi = mipe_connected ? latest_mipe_rssi : 0;
    mipe_status.last_scan_timestamp = k_uptime_get_32();
    mipe_status.connection_attempts = mipe_connection_attempts;

    ble_peripheral_update_mipe_status(&mipe_status);
}

/* BLE Peripheral callbacks */
static void app_connected(void)
{
    log_ble("MotoApp connected");
    motoapp_connected = true;
    update_led1_state();
}

static void app_disconnected(void)
{
    log_ble("MotoApp disconnected");
    motoapp_connected = false;
    streaming_active = false;
    update_led1_state();
    /* Turn off RSSI indicator LED when disconnected */
    gpio_pin_set_dt(&led2, 0);
}

static void streaming_state_changed(bool active)
{
    log_ble("Streaming %s", active ? "started" : "stopped");
    streaming_active = active;
    
    if (!active) {
        /* Turn off RSSI indicator LED when streaming stops */
        gpio_pin_set_dt(&led2, 0);
    }
}

/* BLE Central callback */
static void mipe_rssi_received(int8_t rssi, uint32_t timestamp)
{
    log_ble("Mipe RSSI: %d dBm at %u ms", rssi, timestamp);
    latest_mipe_rssi = rssi;
    latest_mipe_timestamp = timestamp;
    
    /* In beacon mode, we're always "connected" for RSSI purposes */
    if (!mipe_connected) {
        mipe_connected = true;
        log_ble("Mipe beacon detected - RSSI streaming active");
        update_led1_state();
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
        log_ble("TX RSSI: %d dBm (from Mipe)", rssi_to_send);
        
        /* Flash LED2 to indicate RSSI transmission */
        gpio_pin_set_dt(&led2, 1);
        k_timer_start(&led_flash_timer, K_MSEC(200), K_NO_WAIT);
        
        *rssi = rssi_to_send;
        *timestamp = k_uptime_get_32();
        
        return 0;
    } else {
        /* No Mipe beacon detected - don't send any data */
        log_ble("No Mipe beacon detected - skipping RSSI transmission");
        return -ENODATA;
    }
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

    log_ble("=== Host Device v9 Starting ===");
    log_ble("Initial mipe_connected state: %s", mipe_connected ? "true" : "false");
    log_ble("Features: Alternating RSSI + Real Mipe Connection");
    log_ble("LED1 timing fix for proper solid ON state");

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
    log_ble("Heartbeat started (LED0)");

    /* Initialize Bluetooth */
    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return err;
    }
    log_ble("Bluetooth initialized");

    /* Initialize BLE Peripheral (for MotoApp connection) */
    err = ble_peripheral_init(app_connected, app_disconnected, 
                             streaming_state_changed, get_rssi_data);
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

    log_ble("=== Host Device v9 Ready ===");
    log_ble("Advertising as: MIPE_HOST_A1B2");
    log_ble("Scanning for: SinglePing Mipe");
    log_ble("LED Status:");
    log_ble("  LED0: Heartbeat (blinking)");
    log_ble("  LED1: OFF=No connection, Solid=MotoApp only, Flash=Both connected");
    log_ble("  LED2: Flash when transmitting RSSI data");

    /* Main loop */
    while (1) {
        k_sleep(K_SECONDS(1));
        update_mipe_status();
    }

    return 0;
}
