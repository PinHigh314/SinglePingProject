#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include "button_handler.h"

LOG_MODULE_REGISTER(button_handler, LOG_LEVEL_DBG);

/* Button configuration */
#define SW0_NODE DT_ALIAS(sw0)
#if DT_NODE_HAS_STATUS(SW0_NODE, okay)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);
#endif

static struct gpio_callback button_cb_data;
static button_callback_t user_callback = NULL;

static void button_pressed(const struct device *dev, struct gpio_callback *cb,
                          uint32_t pins)
{
    LOG_DBG("Button pressed");
    
    if (user_callback) {
        user_callback();
    }
}

int button_handler_init(button_callback_t callback)
{
    int ret;

#if DT_NODE_HAS_STATUS(SW0_NODE, okay)
    if (!device_is_ready(button.port)) {
        LOG_ERR("Button device not ready");
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret != 0) {
        LOG_ERR("Error configuring button pin: %d", ret);
        return ret;
    }

    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
        LOG_ERR("Error configuring button interrupt: %d", ret);
        return ret;
    }

    gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

    user_callback = callback;
    LOG_INF("Button handler initialized");
    return 0;
#else
    LOG_WRN("No button defined in devicetree");
    return -ENOTSUP;
#endif
}

void button_handler_deinit(void)
{
#if DT_NODE_HAS_STATUS(SW0_NODE, okay)
    gpio_remove_callback(button.port, &button_cb_data);
    user_callback = NULL;
    LOG_INF("Button handler deinitialized");
#endif
}
