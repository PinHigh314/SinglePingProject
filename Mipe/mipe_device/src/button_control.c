#include "button_control.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

// Define the GPIO pins connected to the buttons from the device tree
#define BUTTON0_NODE DT_ALIAS(sw0)
#define BUTTON3_NODE DT_ALIAS(sw3)

static const struct gpio_dt_spec button0 = GPIO_DT_SPEC_GET(BUTTON0_NODE, gpios);
static const struct gpio_dt_spec button3 = GPIO_DT_SPEC_GET(BUTTON3_NODE, gpios);

// Debouncing state variables for SW0
#define DEBOUNCE_DELAY_MS 50

static bool debounced_state = false;
static bool last_raw_state = false;
static int64_t last_change_time = 0;
static bool press_event_detected = false;

// Debouncing state variables for SW3 (battery button)
static bool sw3_debounced_state = false;
static bool sw3_last_raw_state = false;
static int64_t sw3_last_change_time = 0;
static bool sw3_press_event_detected = false;

// Initialize button control system
void button_control_init(void) {
    int ret;

    // Initialize SW0
    if (!gpio_is_ready_dt(&button0)) {
        printk("Error: button0 device %s is not ready\n", button0.port->name);
        return;
    }

    ret = gpio_pin_configure_dt(&button0, GPIO_INPUT);
    if (ret < 0) {
        printk("Error %d: failed to configure button0 device %s pin %d\n",
               ret, button0.port->name, button0.pin);
        return;
    }

    // Initialize SW0 state
    bool initial_reading = gpio_pin_get_dt(&button0) > 0;
    debounced_state = initial_reading;
    last_raw_state = initial_reading;
    last_change_time = k_uptime_get();

    // Initialize SW3
    if (!gpio_is_ready_dt(&button3)) {
        printk("Error: button3 device %s is not ready\n", button3.port->name);
        return;
    }

    ret = gpio_pin_configure_dt(&button3, GPIO_INPUT);
    if (ret < 0) {
        printk("Error %d: failed to configure button3 device %s pin %d\n",
               ret, button3.port->name, button3.pin);
        return;
    }

    // Initialize SW3 state
    bool sw3_initial = gpio_pin_get_dt(&button3) > 0;
    sw3_debounced_state = sw3_initial;
    sw3_last_raw_state = sw3_initial;
    sw3_last_change_time = k_uptime_get();
    
    printk("Button control initialized: SW0 and SW3 ready for power-optimized operation\n");
}

// Update button state machine, to be called from the main loop
void button_control_update(void) {
    int64_t now = k_uptime_get();
    
    // Update SW0
    bool current_raw_state = gpio_pin_get_dt(&button0) > 0;

    // Reset the event flag. It's only true for one call to update().
    press_event_detected = false;

    // If the raw state has changed, reset the timer
    if (current_raw_state != last_raw_state) {
        last_change_time = now;
    }

    // After the debounce delay, if the state is stable, update the debounced state
    if ((now - last_change_time) > DEBOUNCE_DELAY_MS) {
        // If the debounced state is changing from released to pressed
        if (current_raw_state == true && debounced_state == false) {
            press_event_detected = true;
        }
        debounced_state = current_raw_state;
    }

    last_raw_state = current_raw_state;
    
    // Update SW3 (battery button)
    bool sw3_current = gpio_pin_get_dt(&button3) > 0;
    
    // Reset SW3 event flag
    sw3_press_event_detected = false;
    
    // If the raw state has changed, reset the timer
    if (sw3_current != sw3_last_raw_state) {
        sw3_last_change_time = now;
    }
    
    // After the debounce delay, if the state is stable, update the debounced state
    if ((now - sw3_last_change_time) > DEBOUNCE_DELAY_MS) {
        // If the debounced state is changing from released to pressed
        if (sw3_current == true && sw3_debounced_state == false) {
            sw3_press_event_detected = true;
            printk("SW3 pressed - Battery read requested\n");
        }
        sw3_debounced_state = sw3_current;
    }
    
    sw3_last_raw_state = sw3_current;
}

// Returns true for one cycle if a debounced press event was detected on SW0
bool button_was_pressed(void) {
    return press_event_detected;
}

// Returns true for one cycle if SW3 (battery button) was pressed
bool button_sw3_was_pressed(void) {
    return sw3_press_event_detected;
}
