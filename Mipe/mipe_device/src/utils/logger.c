#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "logger.h"

LOG_MODULE_REGISTER(logger, LOG_LEVEL_INF);

int logger_init(void)
{
    LOG_INF("Logger initialized");
    return 0;
}
