#include <stdio.h>
#include <sys/types.h>
#include <errno.h>

#include "headers/coverage.h"

GcovCoverageData parseGcovFile(const char *gcov_filepath) {
    GcovCoverageData coverage_data = {0, 0, 0, 0, 0}; // Initialize parse_error to 0
    FILE *gcov_file = fopen(gcov_filepath, "r");
    if (gcov_file == NULL) {
        perror("Error opening gcov file");
        coverage_data.parse_error = 1; // Indicate file open error
        return coverage_data;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, gcov_file)) != -1) {
        coverage_data.total_lines++;
        char count_str[16]; // Buffer for count string
        int line_num;
        char source_code[1024]; // Buffer for source code line (adjust size if needed)

        int sscanf_result = sscanf(line, "%15[^:]:%d:%[^\n]", count_str, &line_num, source_code);
        if (sscanf_result == 3) {
            if (strcmp(count_str, "-") == 0) {
                coverage_data.non_executable_lines++;
            } else if (strcmp(count_str, "#####") == 0 || strcmp(count_str, "0") == 0) { // Treat "0" as not executed for simplicity
                coverage_data.not_executed_lines++;
            } else {
                // Assume it's an executed line if it's not "-", "#####", or "0" and sscanf succeeded
                coverage_data.executed_lines++;
            }
        } else {
            //fprintf(stderr, "Warning: Could not parse line in gcov file '%s': %s", gcov_filepath, line);
            coverage_data.parse_error = 1; // Indicate parsing error

            
        }
    }

    if (read == -1 && errno != 0) { // Check for getline error other than EOF
        perror("Error reading line from gcov file");
        coverage_data.parse_error = 1; // Indicate getline error
    }

    if (line) { // Check if line was allocated before freeing (getline might fail early)
        free(line); // Free the line buffer allocated by getline
    }

    if (fclose(gcov_file) != 0) {
        perror("Error closing gcov file");
        coverage_data.parse_error = 1; // Indicate file close error
    }

    return coverage_data;
}