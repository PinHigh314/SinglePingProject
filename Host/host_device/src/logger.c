/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "logger.h"

LOG_MODULE_REGISTER(logger_system, LOG_LEVEL_INF);

static enum logger_level current_level = LOGGER_LEVEL_INFO;

void logger_init(enum logger_level level)
{
    current_level = level;
    LOG_INF("Logger initialized with level %d", level);
}

void logger_set_level(enum logger_level level)
{
    current_level = level;
    LOG_INF("Logger level changed to %d", level);
}

enum logger_level logger_get_level(void)
{
    return current_level;
}

void logger_log(enum logger_level level, const char *format, ...)
{
    if (level < current_level) {
        return;
    }

    /* For now, just use Zephyr's logging system */
    switch (level) {
    case LOGGER_LEVEL_DEBUG:
        LOG_DBG("%s", format);
        break;
    case LOGGER_LEVEL_INFO:
        LOG_INF("%s", format);
        break;
    case LOGGER_LEVEL_WARNING:
        LOG_WRN("%s", format);
        break;
    case LOGGER_LEVEL_ERROR:
        LOG_ERR("%s", format);
        break;
    default:
        LOG_INF("%s", format);
        break;
    }
}
