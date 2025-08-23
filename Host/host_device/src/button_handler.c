/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "button_handler.h"

LOG_MODULE_REGISTER(button_handler, LOG_LEVEL_INF);

/* Button configuration */
#define SW0_NODE DT_ALIAS(sw0)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);
static struct gpio_callback button_cb_data;

/* Button callback */
static button_callback_t user_callback = NULL;

static void button_pressed(const struct device *dev, struct gpio_callback *cb,
                          uint32_t pins)
{
    LOG_INF("Button pressed");
    
    if (user_callback) {
        user_callback();
    }
}

int button_handler_init(button_callback_t callback)
{
    int ret;

    if (!gpio_is_ready_dt(&button)) {
        LOG_ERR("Button device not ready");
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret != 0) {
        LOG_ERR("Failed to configure button GPIO: %d", ret);
        return ret;
    }

    ret = gpio_pin_interrupt_configure_dt(&button,
                                         GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
        LOG_ERR("Failed to configure button interrupt: %d", ret);
        return ret;
    }

    gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

    user_callback = callback;

    LOG_INF("Button handler initialized");
    return 0;
}
