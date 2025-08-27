/*
 * TestHost Device - nRF54L15DK
 * Boot sequence with all LEDs flashing and button-initiated BLE connection
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

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

/* Button press handler */
static void button_pressed_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    /* SW1: LED1 flashes once for 200ms */
    if (pins & BIT(button1.pin)) {
        LOG_INF("SW1 pressed - LED1 flash");
        led_on(&led1);
        k_msleep(200);
        led_off(&led1);
    }
    
    /* SW3: Initiate Mipe search and flash LED3 if Mipe is in pairing mode */
    if (pins & BIT(button3.pin)) {
        LOG_INF("SW3 pressed - Initiating Mipe search");
        
        /* Simulate Mipe search - in real implementation, this would be BLE scanning */
        bool mipe_in_pairing_mode = true; /* Placeholder - replace with actual detection */
        
        if (mipe_in_pairing_mode) {
            LOG_INF("Mipe detected in pairing mode - LED3 flash");
            led_on(&led3);
            k_msleep(200);
            led_off(&led3);
        } else {
            LOG_INF("Mipe not found or not in pairing mode");
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
    
    /* Run boot sequence */
    boot_sequence();
    
    LOG_INF("=== System Ready ===");
    LOG_INF("SW1: LED1 flash | SW3: Mipe search");
    
    /* Main loop */
    while (1) {
        k_msleep(1000);
    }
    
    return 0;
}
