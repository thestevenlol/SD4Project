// filepath: /home/jack/College/SD4Project/headers/coverage.h
#ifndef COVERAGE_H
#define COVERAGE_H

#include <signal.h>
#include "generational.h"

// Signal handler for abort signals to flush coverage data
void coverage_signal_handler(int sig);

// Instrumentation functions
extern coverage_t *__coverage_map;
void __coverage_init(void);
void __coverage_reset(void);
void __coverage_log_edge(unsigned int from_id, unsigned int to_id);
void __coverage_dump(void);
int __coverage_count(void);

// Instrumentation injection functions
int instrumentTargetFile(const char *sourcePath, const char *fileName);
int prepareTargetWithCoverage(const char *sourcePath, const char *fileName);

#endif