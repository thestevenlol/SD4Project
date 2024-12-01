#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#define MAX_CMD_LENGTH 100
#define OUTPUT_FILE "inputs.txt"
#define LOG_FILE "lex.log"

/* Error codes */
#define ERR_SUCCESS 0
#define ERR_NULL_INPUT -1
#define ERR_FILE_NOT_FOUND -2
#define ERR_COMMAND_TOO_LONG -3
#define ERR_EXECUTION_FAILED -4
#define ERR_PIPE_FAILED -5

/* Logging function */
static void log_message(const char *message) {
    FILE *log_fp = fopen(LOG_FILE, "a");
    if (log_fp) {
        time_t now = time(NULL);
        char *timestamp = ctime(&now);
        timestamp[strlen(timestamp) - 1] = '\0';  // Remove newline
        fprintf(log_fp, "[%s] %s\n", timestamp, message);
        fclose(log_fp);
    }
}

/**
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