#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void)
{
    int ret;

    if (!gpio_is_ready_dt(&led0)) {
        LOG_ERR("LED0 device not ready");
        return 0;
    }

    ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure LED0: %d", ret);
        return 0;
    }

    LOG_INF("Starting LED heartbeat on LED0");

    while (true) {
        gpio_pin_toggle_dt(&led0);
        k_msleep(500);
    }
    return 0;
}
