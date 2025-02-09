#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>

#include "headers/lex.h"
#include "headers/logger.h"

/**
 * Logs a message to a file with timestamp.
 * 
 * Opens the log file in append mode and writes the given message preceded 
 * by a timestamp in the format [timestamp] message. The timestamp is 
 * generated using the current system time.
 *
 * @param message The message string to be logged
 * 
 * Note: The function silently fails if the log file cannot be opened.
 *       The LOG_FILE macro must be defined before using this function.
 */
static void log_message(const char *message) {
    app_log(LOG_INFO, message);
}

/**
 * @brief 
 * 
 * Generates lexical analyzer using flex
 * @return ERR_SUCCESS on success, error code on failure
 */
int generateLexer() {
    log_message("Attempting to generate lexer");
    
    FILE *pipe = popen("make flex 2>&1", "r");
    if (!pipe) {
        log_message("Failed to execute make flex");
        return ERR_PIPE_FAILED;
    }

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        log_message(buffer);
    }

    int status = pclose(pipe);
    if (status != 0) {
        log_message("Lexer generation failed");
        return ERR_EXECUTION_FAILED;
    }

    log_message("Lexer generated successfully");
    return ERR_SUCCESS;
}

/**
 * Performs lexical analysis on the specified input file
 * @param filename Path to the input file
 * @return ERR_SUCCESS on success, error code on failure
 */
int lexScanFile(const char *filename) {
    char error_msg[256];

    if (!filename) {
        log_message("Null filename provided");
        return ERR_NULL_INPUT;
    }

    // Check if file exists and is readable
    if (access(filename, R_OK) != 0) {
        snprintf(error_msg, sizeof(error_msg), 
                "Input file not accessible: %s", strerror(errno));
        log_message(error_msg);
        return ERR_FILE_NOT_FOUND;
    }

    char command[MAX_CMD_LENGTH];
    if (snprintf(command, sizeof(command), "./scanner < %s > %s", 
                 filename, OUTPUT_FILE) >= sizeof(command)) {
        log_message("Command buffer overflow");
        return ERR_COMMAND_TOO_LONG;
    }

    FILE *pipe = popen(command, "r");
    if (!pipe) {
        snprintf(error_msg, sizeof(error_msg), 
                "Failed to execute scanner: %s", strerror(errno));
        log_message(error_msg);
        return ERR_PIPE_FAILED;
    }

    int status = pclose(pipe);
    if (status != 0) {
        log_message("Scanner execution failed");
        return ERR_EXECUTION_FAILED;
    }

    log_message("Lexical analysis completed successfully");
    return ERR_SUCCESS;
}

/**
 * Extracts minimum and maximum values from input comparisons
 * @param filename The file containing lexer output
 * @return InputRange struct with results
 */
struct InputRange extractInputRange(const char *filename) {
    struct InputRange result = {
        .min = INT_MAX,
        .max = INT_MIN,
        .count = 0,
        .valid = 0
    };
    
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        log_message("Failed to open file for input range extraction");
        return result;
    }

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "input != ")) {
            char *ptr = strstr(line, "input != ");
            ptr += strlen("input != ");
            
            int value;
            if (sscanf(ptr, "%d", &value) == 1) {
                result.min = (value < result.min) ? value : result.min;
                result.max = (value > result.max) ? value : result.max;
                result.count++;
                result.valid = 1;
            }
        }
    }

    fclose(fp);
    
    char logMsg[100];
    snprintf(logMsg, sizeof(logMsg), 
             "Extracted input range: min=%d, max=%d, count=%d",
             result.min, result.max, result.count);
    log_message(logMsg);
    
    return result;
}