#ifndef LOGGER_H
#define LOGGER_H

// Logging levels
#define LOG_INFO "[INFO] "
#define LOG_ERROR "[ERROR] "
#define LOG_DEBUG "[DEBUG] "

void app_log(const char* level, const char* message);
void app_log_with_value(const char* level, const char* message, const char* format, ...);

#endif