#ifndef LEX_H
#define LEX_H

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

/* Structure to hold the min/max results */
struct InputRange {
    int min;
    int max;
    int count;  // Number of comparisons found
    int valid;  // Flag indicating if values were found
};

static void log_message(const char *message);
int generateLexer();
int lexScanFile(const char *filename);
struct InputRange extractInputRange(const char *filename);

#endif