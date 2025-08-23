#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(timer_utils, LOG_LEVEL_DBG);

/* Use the RTC counter for high-precision timing */
#define TIMER_NODE DT_NODELABEL(rtc0)
#if DT_NODE_HAS_STATUS(TIMER_NODE, okay)
static const struct device *const timer_dev = DEVICE_DT_GET(TIMER_NODE);
#else
static const struct device *const timer_dev = NULL;
#endif

static uint32_t timer_frequency = 0;
static bool timer_initialized = false;

int timer_utils_init(void)
{
    if (timer_initialized) {
        LOG_WRN("Timer utils already initialized");
        return 0;
    }

#if DT_NODE_HAS_STATUS(TIMER_NODE, okay)
    if (!device_is_ready(timer_dev)) {
        LOG_ERR("Timer device not ready");
        return -ENODEV;
    }

    timer_frequency = counter_get_frequency(timer_dev);
    if (timer_frequency == 0) {
        LOG_ERR("Failed to get timer frequency");
        return -EINVAL;
    }

    counter_start(timer_dev);
    timer_initialized = true;

    LOG_INF("Timer utils initialized (frequency: %u Hz)", timer_frequency);
    return 0;
#else
    LOG_WRN("No timer device available, using k_uptime");
    timer_initialized = true;
    return 0;
#endif
}

uint64_t timer_utils_get_timestamp_us(void)
{
    if (!timer_initialized) {
        LOG_WRN("Timer not initialized, using k_uptime");
        return k_uptime_get() * 1000;
    }

#if DT_NODE_HAS_STATUS(TIMER_NODE, okay)
    uint32_t ticks;
    int ret = counter_get_value(timer_dev, &ticks);
    if (ret != 0) {
        LOG_ERR("Failed to read counter value");
        return k_uptime_get() * 1000;
    }

    /* Convert ticks to microseconds */
    uint64_t us = ((uint64_t)ticks * 1000000ULL) / timer_frequency;
    return us;
#else
    return k_uptime_get() * 1000;
#endif
}

uint32_t timer_utils_get_timestamp_ms(void)
{
    return (uint32_t)(timer_utils_get_timestamp_us() / 1000);
}

void timer_utils_delay_us(uint32_t us)
{
    if (us < 1000) {
        /* For very short delays, use busy wait */
        k_busy_wait(us);
    } else {
        /* For longer delays, use kernel sleep */
        k_usleep(us);
    }
}

void timer_utils_delay_ms(uint32_t ms)
{
    k_msleep(ms);
}

uint32_t timer_utils_calculate_diff_us(uint64_t start, uint64_t end)
{
    if (end < start) {
        /* Handle wraparound */
        return (uint32_t)(UINT64_MAX - start + end + 1);
    }
    return (uint32_t)(end - start);
}

uint32_t timer_utils_calculate_diff_ms(uint32_t start, uint32_t end)
{
    if (end < start) {
        /* Handle wraparound */
        return (UINT32_MAX - start + end + 1);
    }
    return (end - start);
}

void timer_utils_deinit(void)
{
    if (!timer_initialized) {
        return;
    }

#if DT_NODE_HAS_STATUS(TIMER_NODE, okay)
    counter_stop(timer_dev);
#endif

    timer_initialized = false;
    LOG_INF("Timer utils deinitialized");
}

/* High-resolution stopwatch functionality */
struct stopwatch {
    uint64_t start_time;
    uint64_t total_time;
    bool running;
};

static struct stopwatch stopwatches[4];

void timer_utils_stopwatch_start(uint8_t id)
{
    if (id >= ARRAY_SIZE(stopwatches)) {
        LOG_ERR("Invalid stopwatch ID: %u", id);
        return;
    }

    struct stopwatch *sw = &stopwatches[id];
    sw->start_time = timer_utils_get_timestamp_us();
    sw->running = true;
    LOG_DBG("Stopwatch %u started", id);
}

uint32_t timer_utils_stopwatch_stop(uint8_t id)
{
    if (id >= ARRAY_SIZE(stopwatches)) {
        LOG_ERR("Invalid stopwatch ID: %u", id);
        return 0;
    }

    struct stopwatch *sw = &stopwatches[id];
    if (!sw->running) {
        LOG_WRN("Stopwatch %u not running", id);
        return 0;
    }

    uint64_t end_time = timer_utils_get_timestamp_us();
    uint32_t elapsed = timer_utils_calculate_diff_us(sw->start_time, end_time);
    sw->total_time += elapsed;
    sw->running = false;

    LOG_DBG("Stopwatch %u stopped: %u us", id, elapsed);
    return elapsed;
}

void timer_utils_stopwatch_reset(uint8_t id)
{
    if (id >= ARRAY_SIZE(stopwatches)) {
        LOG_ERR("Invalid stopwatch ID: %u", id);
        return;
    }

    memset(&stopwatches[id], 0, sizeof(struct stopwatch));
    LOG_DBG("Stopwatch %u reset", id);
}
