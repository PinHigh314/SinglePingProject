#include "led_control.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(led_control, LOG_LEVEL_INF);

// Array of LED specifications from the device tree
// Map multiple logical LEDs to available physical LEDs (we only have 4)
// MIPE LED mapping: LED0=Heartbeat, LED1=Connection, LED3=Battery/Data
static const struct gpio_dt_spec leds[] = {
	[LED_ID_HEARTBEAT]  = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
	[LED_ID_PAIRING]    = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),  // Not used on MIPE
	[LED_ID_CONNECTION] = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),  // LED1 for connection status
	[LED_ID_DATA]       = GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios),
	[LED_ID_ERROR]      = GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios), // Share with DATA LED
};

struct led_state {
	enum led_pattern current_pattern;
	int64_t last_toggle_time;
	bool current_state;  // Track current on/off state
};

static struct led_state led_states[LED_ID_COUNT];

// Initialize LED control system
void led_control_init(void)
{
	LOG_INF("Initializing LED control system");
	
	for (int i = 0; i < LED_ID_COUNT; i++) {
		if (!gpio_is_ready_dt(&leds[i])) {
			LOG_ERR("LED device %s is not ready", leds[i].port->name);
			continue; // Try to initialize other LEDs
		}

		int ret = gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure LED %d (error %d)", i, ret);
			continue; // Try to initialize other LEDs
		}
		
		led_states[i].current_pattern = LED_PATTERN_OFF;
		led_states[i].last_toggle_time = 0;
		led_states[i].current_state = false;
		
		LOG_DBG("LED %d initialized", i);
	}
	
	LOG_INF("LED control system initialized");
}

// Set LED state
void led_set_state(enum led_id id, bool on)
{
	if (id >= LED_ID_COUNT) return;
	led_set_pattern(id, on ? LED_PATTERN_ON : LED_PATTERN_OFF);
}

// Set a new LED pattern
void led_set_pattern(enum led_id id, enum led_pattern pattern)
{
	if (id >= LED_ID_COUNT) return;

	led_states[id].current_pattern = pattern;
	led_states[id].last_toggle_time = k_uptime_get(); // Reset timer on pattern change

	// Apply initial state immediately
	if (led_states[id].current_pattern == LED_PATTERN_OFF) {
		gpio_pin_set_dt(&leds[id], 0);
	} else if (led_states[id].current_pattern == LED_PATTERN_ON) {
		gpio_pin_set_dt(&leds[id], 1);
	}
}

// Update LED pattern state machine, called from main loop
void led_control_update(void)
{
	for (int i = 0; i < LED_ID_COUNT; i++) {
		int32_t period_ms = 0;
		int64_t now = k_uptime_get();

		switch (led_states[i].current_pattern) {
		case LED_PATTERN_OFF:
			gpio_pin_set_dt(&leds[i], 0);
			continue;
		case LED_PATTERN_ON:
		case LED_PATTERN_CONNECTED:
			gpio_pin_set_dt(&leds[i], 1);
			continue;
		case LED_PATTERN_HEARTBEAT:
			period_ms = 1000; // 1000ms blink interval (500ms on, 500ms off)
			break;
		case LED_PATTERN_ADVERTISING:
			period_ms = 400; // 400ms blink interval (200ms on, 200ms off)
			break;
		case LED_PATTERN_DATA_ACTIVE:
			period_ms = 100; // 100ms blink interval (50ms on, 50ms off)
			break;
		case LED_PATTERN_ERROR:
			period_ms = 200; // 200ms blink interval (100ms on, 100ms off)
			break;
		case LED_PATTERN_SLOW_BLINK:
			period_ms = 2000; // 2000ms blink interval (1000ms on, 1000ms off)
			break;
		default:
			continue;
		}

		// Handle blinking patterns
		if (period_ms > 0 && (now - led_states[i].last_toggle_time >= (period_ms / 2))) {
			led_states[i].current_state = !led_states[i].current_state;
			gpio_pin_set_dt(&leds[i], led_states[i].current_state ? 1 : 0);
			led_states[i].last_toggle_time = now;
		}
	}
}
