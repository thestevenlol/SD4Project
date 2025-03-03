#ifndef COVERAGE_H
#define COVERAGE_H

#include "generational.h"

// Instrumentation functions
extern coverage_t* __coverage_map;
void __coverage_init(void);
void __coverage_reset(void);
void __coverage_log_edge(unsigned int from_id, unsigned int to_id);
void __coverage_dump(void);
int __coverage_count(void);

// Instrumentation injection functions
int instrumentTargetFile(const char* sourcePath, const char* fileName);
int prepareTargetWithCoverage(const char* sourcePath, const char* fileName);

#endif