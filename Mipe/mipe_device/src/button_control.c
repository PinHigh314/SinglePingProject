#include "button_control.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

// Define the GPIO pin connected to the button from the device tree
#define BUTTON0_NODE DT_ALIAS(sw0)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON0_NODE, gpios);

// Debouncing state variables
#define DEBOUNCE_DELAY_MS 50

static bool debounced_state = false;
static bool last_raw_state = false;
static int64_t last_change_time = 0;
static bool press_event_detected = false;

// Initialize button control system
void button_control_init(void) {
    int ret;

    if (!gpio_is_ready_dt(&button)) {
        printk("Error: button device %s is not ready\n", button.port->name);
        return;
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret < 0) {
        printk("Error %d: failed to configure button device %s pin %d\n",
               ret, button.port->name, button.pin);
        return;
    }

    // Initialize state
    bool initial_reading = gpio_pin_get_dt(&button) > 0;
    debounced_state = initial_reading;
    last_raw_state = initial_reading;
    last_change_time = k_uptime_get();
}

// Update button state machine, to be called from the main loop
void button_control_update(void) {
    bool current_raw_state = gpio_pin_get_dt(&button) > 0;
    int64_t now = k_uptime_get();

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
}

// Returns true for one cycle if a debounced press event was detected
bool button_was_pressed(void) {
    return press_event_detected;
}
