#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#include "headers/hash.h"

// Function to generate the SHA-256 hash of a file
char* generate_program_hash(const char* file_path) {
    char command[256];
    char hash[65]; // SHA-256 hash is 64 characters long plus a null terminator

    // Construct the command to call the shell script
    snprintf(command, sizeof(command), "./hash.sh %s", file_path);

    // Open a pipe to the command
    FILE* pipe = popen(command, "r");
    if (!pipe) {
        perror("popen");
        return NULL;
    }

    // Read the hash from the pipe
    if (fgets(hash, sizeof(hash), pipe) == NULL) {
        perror("fgets");
        pclose(pipe);
        return NULL;
    }

    // Close the pipe
    pclose(pipe);

    // Remove the newline character from the hash
    hash[strcspn(hash, "\n")] = '\0';

    // Allocate memory for the hash and return it
    char* result = strdup(hash);
    if (!result) {
        perror("strdup");
    }
    return result;
}

char* get_full_path(const char* file_path) {
    char* full_path = (char*)malloc(PATH_MAX);
    if (!full_path) {
        perror("malloc");
        return NULL;
    }

    if (realpath(file_path, full_path) == NULL) {
        perror("realpath");
        free(full_path);
        return NULL;
    }

    return full_path;
}


char* get_current_time() {
    time_t now = time(NULL);
    if (now == -1) {
        perror("time");
        return NULL;
    }

    struct tm* tm_info = localtime(&now);
    if (!tm_info) {
        perror("localtime");
        return NULL;
    }

    char* time_str = (char*)malloc(80); // Buffer to hold the time string
    if (!time_str) {
        perror("malloc");
        return NULL;
    }

    if (strftime(time_str, 80, "%a %b %d %H:%M:%S %Y", tm_info) == 0) {
        perror("strftime");
        free(time_str);
        return NULL;
    }

    return time_str;
}