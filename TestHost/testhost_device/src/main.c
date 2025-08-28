/*
 * TestHost Device - nRF54L15DK
 * Boot sequence with all LEDs flashing and button-initiated BLE connection
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/hci.h>

LOG_MODULE_REGISTER(testhost, LOG_LEVEL_INF);

/* LED Definitions - Using proper Zephyr device tree aliases */
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1) 
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);

/* Button Definitions - Using proper Zephyr device tree aliases */
#define BUTTON0_NODE DT_ALIAS(sw0)
#define BUTTON1_NODE DT_ALIAS(sw1)
#define BUTTON2_NODE DT_ALIAS(sw2) 
#define BUTTON3_NODE DT_ALIAS(sw3)

static const struct gpio_dt_spec button0 = GPIO_DT_SPEC_GET(BUTTON0_NODE, gpios);
static const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET(BUTTON1_NODE, gpios);
static const struct gpio_dt_spec button2 = GPIO_DT_SPEC_GET(BUTTON2_NODE, gpios);
static const struct gpio_dt_spec button3 = GPIO_DT_SPEC_GET(BUTTON3_NODE, gpios);

static struct gpio_callback button0_cb_data;
static struct gpio_callback button1_cb_data;
static struct gpio_callback button2_cb_data;
static struct gpio_callback button3_cb_data;

/* Work structure for LED3 flashing */
static struct k_work led3_flash_work;

/* Work structure for LED1 flashing */
static struct k_work led1_flash_work;

/* Work structure for BLE scanning */
static struct k_work ble_scan_work;

/* Work structure for BLE disconnection */
static struct k_work ble_disconnect_work;

/* Delayed work for BLE connection */
static struct k_work_delayable ble_connect_work;

/* Timer for reconnection cooldown */
static struct k_timer reconnect_timer;

/* BLE connection handle */
static struct bt_conn *current_conn;

/* Reconnection flag */
static bool can_reconnect = true;

/* Connection lock */
static atomic_t is_connecting = ATOMIC_INIT(false);

/* Address of the device to connect to */
static bt_addr_le_t connect_addr;

/* MIPE device name to scan for */
#define MIPE_DEVICE_NAME "MIPE"

/* BLE scan parameters */
static struct bt_le_scan_param scan_params = {
    .type = BT_LE_SCAN_TYPE_ACTIVE,
    .options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
    .interval = BT_GAP_SCAN_FAST_INTERVAL,
    .window = BT_GAP_SCAN_FAST_WINDOW,
};

/* LED control functions */
static void led_on(const struct gpio_dt_spec *led)
{
    gpio_pin_set_dt(led, 1);
}

static void led_off(const struct gpio_dt_spec *led)
{
    gpio_pin_set_dt(led, 0);
}

static void all_leds_on(void)
{
    led_on(&led0);
    led_on(&led1);
    led_on(&led2);
    led_on(&led3);
}

static void all_leds_off(void)
{
    led_off(&led0);
    led_off(&led1);
    led_off(&led2);
    led_off(&led3);
}

/* Boot sequence - all LEDs flash 3 times */
static void boot_sequence(void)
{
    for (int i = 0; i < 3; i++) {
        all_leds_on();
        k_msleep(200);
        all_leds_off();
        k_msleep(200);
    }
    LOG_INF("Boot sequence complete");
}

/* LED3 flashing work handler */
static void led3_flash_work_handler(struct k_work *work)
{
    LOG_INF("WORKQUEUE: Flashing LED3");
    led_on(&led3);
    k_msleep(1000);
    led_off(&led3);
    LOG_INF("WORKQUEUE: LED3 flash completed");
}

/* LED1 flashing work handler */
static void led1_flash_work_handler(struct k_work *work)
{
    LOG_INF("WORKQUEUE: Flashing LED1");
    led_on(&led1);
    k_msleep(200);
    led_off(&led1);
    LOG_INF("WORKQUEUE: LED1 flash completed");
}

/* Reconnection timer handler */
static void reconnect_timer_handler(struct k_timer *timer_id)
{
    can_reconnect = true;
    LOG_INF("Reconnection enabled");
}

/* BLE connect work handler */
static void ble_connect_work_handler(struct k_work *work)
{
    // Release any lingering connection object for this address
    struct bt_conn *old_conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &connect_addr);
    if (old_conn) {
        bt_conn_unref(old_conn);
    }

    struct bt_conn_le_create_param create_param = {
        .options = BT_CONN_LE_OPT_NONE,
        .interval = BT_GAP_SCAN_FAST_INTERVAL,
        .window = BT_GAP_SCAN_FAST_WINDOW,
        .interval_coded = 0,
        .window_coded = 0,
        .timeout = 0,
    };

    int err = bt_conn_le_create(&connect_addr, &create_param, BT_LE_CONN_PARAM_DEFAULT, &current_conn);
    if (err) {
        LOG_ERR("Failed to create connection: %d", err);
        if (current_conn) {
            bt_conn_unref(current_conn);
            current_conn = NULL;
        }
        atomic_set(&is_connecting, false); // Reset on failure
    } else {
        LOG_INF("Connecting to MIPE device...");
    }
}

/* BLE scan callback function */
static void scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
                    struct net_buf_simple *buf)
{
    char addr_str[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

    /* Check for MIPE device by name in both advertising and scan response */
    if (adv_type == BT_GAP_ADV_TYPE_ADV_IND || adv_type == BT_GAP_ADV_TYPE_SCAN_RSP) {
        /* Manual parsing of advertising data to find device name */
        struct net_buf_simple_state state;
        struct bt_data data;
        const char *found_name = NULL;
        
        /* Save buffer state for restoration */
        net_buf_simple_save(buf, &state);
        
        for (int i = 0; i < buf->len; ) {
            uint8_t len = net_buf_simple_pull_u8(buf);
            if (len == 0) break;
            
            data.type = net_buf_simple_pull_u8(buf);
            data.data_len = len - 1;
            data.data = buf->data;
            
            if (data.type == BT_DATA_NAME_SHORTENED || data.type == BT_DATA_NAME_COMPLETE) {
                if (data.data_len >= strlen(MIPE_DEVICE_NAME)) {
                    found_name = (const char *)data.data;
                    /* Check if name matches */
                    if (strncmp(found_name, MIPE_DEVICE_NAME, strlen(MIPE_DEVICE_NAME)) == 0) {
                        break;
                    }
                }
                found_name = NULL;
            }
            
            net_buf_simple_pull(buf, data.data_len);
        }
        
        /* Restore buffer state */
        net_buf_simple_restore(buf, &state);
        
        if (found_name && !atomic_get(&is_connecting)) {
            atomic_set(&is_connecting, true);
            LOG_INF("MIPE device found: %s, RSSI: %d, Adv type: %d", addr_str, rssi, adv_type);
            
            /* Stop scanning and schedule connection */
            bt_le_scan_stop();
            bt_addr_le_copy(&connect_addr, addr);
            k_work_schedule(&ble_connect_work, K_MSEC(50));
        }
    }
}

/* BLE disconnect work handler */
static void ble_disconnect_work_handler(struct k_work *work)
{
    if (current_conn) {
        LOG_INF("Disconnecting from MIPE device via work queue");
        bt_conn_disconnect(current_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    }
}

/* BLE scan work handler */
static void ble_scan_work_handler(struct k_work *work)
{
    LOG_INF("Starting BLE scan for MIPE device...");
    
    /* Stop any existing scan first */
    bt_le_scan_stop();
    k_msleep(50);
    
    int err = bt_le_scan_start(&scan_params, scan_cb);
    if (err) {
        LOG_ERR("Failed to start scanning: %d", err);
        /* Retry after delay */
        k_msleep(1000);
        err = bt_le_scan_start(&scan_params, scan_cb);
        if (err) {
            LOG_ERR("Second scan attempt failed: %d", err);
            return;
        }
    }
    
    LOG_INF("BLE scan active - looking for MIPE devices");
    
    /* Scan for 5 seconds then stop */
    k_msleep(5000);
    bt_le_scan_stop();
    LOG_INF("BLE scan completed - no MIPE device found");
}


/* BLE connection callback */
static void connected(struct bt_conn *conn, uint8_t err)
{
    atomic_set(&is_connecting, false);
    if (err) {
        LOG_ERR("Connection failed: %d", err);
        return;
    }
    
    current_conn = bt_conn_ref(conn);
    LOG_INF("Connected to MIPE device");
    
    /* Flash LED3 to indicate successful connection */
    k_work_submit(&led3_flash_work);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected: reason %d", reason);

    if (current_conn) {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }
    atomic_set(&is_connecting, false);
    can_reconnect = false;
    /* Start a timer or delay to re-enable reconnection */
    k_timer_start(&reconnect_timer, K_SECONDS(2), K_NO_WAIT);
}

/* BLE connection callbacks */
BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

/* Button press handler */
static void button_pressed_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    /* SW1: LED1 flashes once for 200ms */
    if (pins & BIT(button1.pin)) {
        LOG_INF("SW1 pressed - LED1 flash");
        k_work_submit(&led1_flash_work);
    }
    
        /* SW3: Toggle Mipe connection - connect if disconnected, disconnect if connected */
        if (pins & BIT(button3.pin)) {
            if (current_conn) {
                LOG_INF("SW3 pressed - Scheduling disconnect via work queue");
                k_work_submit(&ble_disconnect_work);
            } else if (can_reconnect) {
                LOG_INF("SW3 pressed - Initiating Mipe search");
                LOG_INF("DEBUG: Button3 interrupt triggered successfully");
                
                /* Start BLE scanning for MIPE device */
                k_work_submit(&ble_scan_work);
            } else {
                LOG_WRN("SW3 pressed - Reconnection not allowed yet");
            }
        }
}

/* Initialize LEDs */
static int init_leds(void)
{
    const struct gpio_dt_spec *leds[] = {&led0, &led1, &led2, &led3};
    
    for (int i = 0; i < 4; i++) {
        if (!gpio_is_ready_dt(leds[i])) {
            LOG_ERR("LED %d device not ready", i);
            return -ENODEV;
        }
        
        int ret = gpio_pin_configure_dt(leds[i], GPIO_OUTPUT_INACTIVE);
        if (ret < 0) {
            LOG_ERR("Failed to configure LED %d: %d", i, ret);
            return ret;
        }
    }
    
    LOG_INF("All LEDs initialized successfully");
    return 0;
}

/* Initialize buttons */
static int init_buttons(void)
{
    const struct gpio_dt_spec buttons[] = {button0, button1, button2, button3};
    struct gpio_callback *callbacks[] = {&button0_cb_data, &button1_cb_data, 
                                        &button2_cb_data, &button3_cb_data};
    
    for (int i = 0; i < 4; i++) {
        if (!gpio_is_ready_dt(&buttons[i])) {
            LOG_WRN("Button %d device not ready - skipping", i);
            continue;
        }
        
        int ret = gpio_pin_configure_dt(&buttons[i], GPIO_INPUT | GPIO_PULL_UP);
        if (ret < 0) {
            LOG_WRN("Failed to configure button %d: %d - skipping", i, ret);
            continue;
        }
        
        ret = gpio_pin_interrupt_configure_dt(&buttons[i], GPIO_INT_EDGE_FALLING);
        if (ret < 0) {
            LOG_WRN("Failed to configure button %d interrupt: %d - skipping", i, ret);
            continue;
        }
        
        gpio_init_callback(callbacks[i], button_pressed_handler, BIT(buttons[i].pin));
        gpio_add_callback(buttons[i].port, callbacks[i]);
        
        LOG_INF("Button %d initialized successfully", i);
    }
    
    return 0;
}

int main(void)
{
    LOG_INF("=== TestHost Device Starting ===");
    LOG_INF("Board: nRF54L15DK");
    LOG_INF("Purpose: LED boot sequence + button-initiated BLE connection");
    
    /* Initialize hardware */
    if (init_leds() != 0) {
        LOG_ERR("Failed to initialize LEDs");
        return -1;
    }
    
    if (init_buttons() != 0) {
        LOG_WRN("Some buttons failed to initialize - continuing");
    }
    
    /* Initialize work structures */
    k_work_init(&led3_flash_work, led3_flash_work_handler);
    k_work_init(&led1_flash_work, led1_flash_work_handler);
    k_work_init(&ble_scan_work, ble_scan_work_handler);
    k_work_init(&ble_disconnect_work, ble_disconnect_work_handler);
    k_work_init_delayable(&ble_connect_work, ble_connect_work_handler);

    /* Initialize timer */
    k_timer_init(&reconnect_timer, reconnect_timer_handler, NULL);

    /* Initialize Bluetooth */
    int err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed: %d", err);
        return -1;
    }
    LOG_INF("Bluetooth initialized");

    /* Run boot sequence */
    boot_sequence();
    
    LOG_INF("=== System Ready ===");
    LOG_INF("SW1: LED1 flash | SW3: Mipe search + BLE connect");
    /* Main loop */
    while (1) {
        k_msleep(1000);
    }
    return 0;
}
