#ifndef COVERAGE_H
#define COVERAGE_H

typedef struct {
    int executed_lines;
    int not_executed_lines;
    int non_executable_lines;
    int total_lines; 
    int parse_error;
} GcovCoverageData;

GcovCoverageData parseGcovFile(const char *gcov_filepath);

#endif