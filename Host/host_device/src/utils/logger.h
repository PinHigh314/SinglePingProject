#ifndef LOGGER_H
#define LOGGER_H

#include <stddef.h>
#include <stdarg.h>

/* Log levels */
enum logger_level {
    LOGGER_LEVEL_NONE = 0,
    LOGGER_LEVEL_ERROR,
    LOGGER_LEVEL_WARNING,
    LOGGER_LEVEL_INFO,
    LOGGER_LEVEL_DEBUG
};

/**
 * @brief Initialize the logger
 * 
 * @param level Initial log level
 * @return 0 on success, negative error code on failure
 */
int logger_init(enum logger_level level);

/**
 * @brief Set the current log level
 * 
 * @param level New log level
 */
void logger_set_level(enum logger_level level);

/**
 * @brief Get the current log level
 * 
 * @return Current log level
 */
enum logger_level logger_get_level(void);

/**
 * @brief Log a message
 * 
 * @param level Log level of the message
 * @param module Module name
 * @param format Printf-style format string
 * @param ... Variable arguments
 */
void logger_log(enum logger_level level, const char *module, const char *format, ...);

/**
 * @brief Dump the log buffer to a string
 * 
 * @param output Output buffer
 * @param max_size Maximum size of output buffer
 * @return Number of bytes written
 */
int logger_dump(char *output, size_t max_size);

/**
 * @brief Clear the log buffer
 */
void logger_clear(void);

/**
 * @brief Get current buffer usage
 * 
 * @return Number of bytes used in buffer
 */
size_t logger_get_buffer_usage(void);

/**
 * @brief Deinitialize the logger
 */
void logger_deinit(void);

/* Convenience macros */
#define LOGGER_ERROR(module, ...) logger_log(LOGGER_LEVEL_ERROR, module, __VA_ARGS__)
#define LOGGER_WARN(module, ...) logger_log(LOGGER_LEVEL_WARNING, module, __VA_ARGS__)
#define LOGGER_INFO(module, ...) logger_log(LOGGER_LEVEL_INFO, module, __VA_ARGS__)
#define LOGGER_DEBUG(module, ...) logger_log(LOGGER_LEVEL_DEBUG, module, __VA_ARGS__)

#endif /* LOGGER_H */
