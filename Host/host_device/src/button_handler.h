/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <stdint.h>

/* Button callback function type */
typedef void (*button_callback_t)(void);

/**
 * @brief Initialize button handler
 * 
 * @param callback Function to call when button is pressed
 * @return 0 on success, negative error code on failure
 */
int button_handler_init(button_callback_t callback);

#endif /* BUTTON_HANDLER_H */
