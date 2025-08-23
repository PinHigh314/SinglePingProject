/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>

/* Logger levels */
enum logger_level {
    LOGGER_LEVEL_DEBUG = 0,
    LOGGER_LEVEL_INFO,
    LOGGER_LEVEL_WARNING,
    LOGGER_LEVEL_ERROR,
};

/**
 * @brief Initialize logger
 * 
 * @param level Initial logging level
 */
void logger_init(enum logger_level level);

/**
 * @brief Set logger level
 * 
 * @param level New logging level
 */
void logger_set_level(enum logger_level level);

/**
 * @brief Get current logger level
 * 
 * @return Current logging level
 */
enum logger_level logger_get_level(void);

/**
 * @brief Log a message
 * 
 * @param level Message level
 * @param format Printf-style format string
 * @param ... Variable arguments
 */
void logger_log(enum logger_level level, const char *format, ...);

#endif /* LOGGER_H */
