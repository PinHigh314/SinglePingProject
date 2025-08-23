#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <stdint.h>

/* Button callback function type */
typedef void (*button_callback_t)(void);

/**
 * @brief Initialize the button handler
 * 
 * @param callback Callback function to be called when button is pressed
 * @return 0 on success, negative error code on failure
 */
int button_handler_init(button_callback_t callback);

/**
 * @brief Deinitialize the button handler
 */
void button_handler_deinit(void);

#endif /* BUTTON_HANDLER_H */
