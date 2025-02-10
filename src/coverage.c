#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include "../headers/coverage.h"
#include "../headers/logger.h"

GcovCoverageData parseGcovFile(const char* filename) {
    GcovCoverageData coverage = {0, 0};
    
    // Change to coverage directory where gcov files are generated
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        if (chdir("coverage") == 0) {
            FILE* file = fopen("Problem10.c.gcov", "r");
            
            if (!file) {
                app_log_with_value(LOG_ERROR, "Failed to open gcov file:", "%s", "Problem10.c.gcov");
                chdir(cwd);
                return coverage;
            }

            char *line = NULL;
            size_t len = 0;
            ssize_t read;

            while ((read = getline(&line, &len, file)) != -1) {
                if (line[0] == ' ' || line[0] == '#' || line[0] == '-') {
                    char count_str[256];
                    if (sscanf(line, "%255s", count_str) == 1) {
                        if (strcmp(count_str, "#####") == 0) {
                            coverage.not_executed_lines++;
                        } else if (strcmp(count_str, "-") != 0) {
                            // Check if it's a number without storing it
                            char *endptr;
                            strtol(count_str, &endptr, 10);
                            if (endptr != count_str) { // Conversion succeeded
                                coverage.executed_lines++;
                            }
                        }
                    }
                }
            }

            if (line) {
                free(line);
            }
            fclose(file);
            chdir(cwd);

            app_log_with_value(LOG_DEBUG, "Coverage analysis:", "Executed: %d, Not executed: %d", 
                          coverage.executed_lines, coverage.not_executed_lines);
        }
    }

    return coverage;
}