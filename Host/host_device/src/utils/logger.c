#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "logger.h"

LOG_MODULE_REGISTER(custom_logger, LOG_LEVEL_DBG);

#define LOG_BUFFER_SIZE 4096
#define MAX_LOG_LINE_SIZE 256

/* Ring buffer for storing log messages */
RING_BUF_DECLARE(log_ring_buf, LOG_BUFFER_SIZE);

/* Mutex for thread-safe access */
K_MUTEX_DEFINE(log_mutex);

static bool logger_initialized = false;
static enum logger_level current_log_level = LOGGER_LEVEL_INFO;

int logger_init(enum logger_level level)
{
    if (logger_initialized) {
        LOG_WRN("Logger already initialized");
        return -EALREADY;
    }

    current_log_level = level;
    ring_buf_reset(&log_ring_buf);
    logger_initialized = true;

    LOG_INF("Custom logger initialized with level %d", level);
    return 0;
}

void logger_set_level(enum logger_level level)
{
    if (level >= LOGGER_LEVEL_NONE && level <= LOGGER_LEVEL_DEBUG) {
        current_log_level = level;
        LOG_DBG("Log level set to %d", level);
    }
}

enum logger_level logger_get_level(void)
{
    return current_log_level;
}

static const char *level_to_string(enum logger_level level)
{
    switch (level) {
    case LOGGER_LEVEL_ERROR:
        return "ERROR";
    case LOGGER_LEVEL_WARNING:
        return "WARN";
    case LOGGER_LEVEL_INFO:
        return "INFO";
    case LOGGER_LEVEL_DEBUG:
        return "DEBUG";
    default:
        return "UNKNOWN";
    }
}

void logger_log(enum logger_level level, const char *module, const char *format, ...)
{
    if (!logger_initialized || level > current_log_level) {
        return;
    }

    char buffer[MAX_LOG_LINE_SIZE];
    va_list args;
    int len;

    /* Format timestamp */
    uint32_t timestamp = k_uptime_get_32();
    len = snprintf(buffer, sizeof(buffer), "[%8u.%03u] [%s] %s: ",
                   timestamp / 1000, timestamp % 1000,
                   level_to_string(level), module);

    /* Format the actual message */
    va_start(args, format);
    len += vsnprintf(buffer + len, sizeof(buffer) - len, format, args);
    va_end(args);

    /* Add newline if not present */
    if (len < sizeof(buffer) - 1 && buffer[len - 1] != '\n') {
        buffer[len++] = '\n';
        buffer[len] = '\0';
    }

    /* Store in ring buffer */
    k_mutex_lock(&log_mutex, K_FOREVER);
    ring_buf_put(&log_ring_buf, (uint8_t *)buffer, len);
    k_mutex_unlock(&log_mutex);

    /* Also print to console */
    printk("%s", buffer);
}

int logger_dump(char *output, size_t max_size)
{
    if (!logger_initialized || !output || max_size == 0) {
        return -EINVAL;
    }

    k_mutex_lock(&log_mutex, K_FOREVER);
    
    size_t bytes_read = ring_buf_get(&log_ring_buf, (uint8_t *)output, max_size - 1);
    output[bytes_read] = '\0';
    
    k_mutex_unlock(&log_mutex);

    return bytes_read;
}

void logger_clear(void)
{
    if (!logger_initialized) {
        return;
    }

    k_mutex_lock(&log_mutex, K_FOREVER);
    ring_buf_reset(&log_ring_buf);
    k_mutex_unlock(&log_mutex);

    LOG_DBG("Log buffer cleared");
}

size_t logger_get_buffer_usage(void)
{
    if (!logger_initialized) {
        return 0;
    }

    k_mutex_lock(&log_mutex, K_FOREVER);
    size_t usage = ring_buf_size_get(&log_ring_buf);
    k_mutex_unlock(&log_mutex);

    return usage;
}

void logger_deinit(void)
{
    if (!logger_initialized) {
        return;
    }

    logger_clear();
    logger_initialized = false;
    LOG_INF("Custom logger deinitialized");
}
