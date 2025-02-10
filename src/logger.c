#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include "../headers/logger.h"

void app_log(const char* level, const char* message) {
    time_t now;
    time(&now);
    char* timestamp = ctime(&now);
    timestamp[strlen(timestamp) - 1] = '\0'; // Remove newline
    printf("%s %s%s\n", timestamp, level, message);
}

void app_log_with_value(const char* level, const char* message, const char* format, ...) {
    time_t now;
    time(&now);
    char* timestamp = ctime(&now);
    timestamp[strlen(timestamp) - 1] = '\0';
    
    printf("%s %s%s ", timestamp, level, message);
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}