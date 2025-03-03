#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../headers/coverage.h"
#include "../headers/generational.h"

// Global coverage map used by instrumented code
coverage_t* __coverage_map = NULL;

// Initialize coverage map
void __coverage_init(void) {
    if (__coverage_map == NULL) {
        __coverage_map = calloc(COVERAGE_MAP_SIZE, sizeof(coverage_t));
        if (!__coverage_map) {
            fprintf(stderr, "Failed to allocate memory for coverage map\n");
            exit(1);
        }
    }
}

// Reset coverage map to all zeros
void __coverage_reset(void) {
    if (__coverage_map) {
        memset(__coverage_map, 0, COVERAGE_MAP_SIZE * sizeof(coverage_t));
    }
}

// Log an edge transition in the coverage map
void __coverage_log_edge(unsigned int from_id, unsigned int to_id) {
    if (!__coverage_map) {
        __coverage_init();
    }
    
    // Create a unique edge ID using both block IDs
    unsigned int edge_id = (from_id ^ to_id) % COVERAGE_MAP_SIZE;
    
    // Increment hit count, saturating at 255
    if (__coverage_map[edge_id] < 255) {
        __coverage_map[edge_id]++;
    }
}

// Dump coverage information for debugging
void __coverage_dump(void) {
    if (!__coverage_map) {
        printf("Coverage map not initialized\n");
        return;
    }
    
    int covered = 0;
    for (int i = 0; i < COVERAGE_MAP_SIZE; i++) {
        if (__coverage_map[i] > 0) {
            covered++;
        }
    }
    
    printf("Coverage summary: %d of %d edges covered\n", covered, COVERAGE_MAP_SIZE);
}

// Count the number of edges covered
int __coverage_count(void) {
    if (!__coverage_map) {
        return 0;
    }
    
    int covered = 0;
    for (int i = 0; i < COVERAGE_MAP_SIZE; i++) {
        if (__coverage_map[i] > 0) {
            covered++;
        }
    }
    
    return covered;
}

// Compile-time instrumentation using source code modification
int instrumentTargetFile(const char* sourcePath, const char* fileName) {
    char sourceFilePath[1024];
    char tempFilePath[1024];
    char command[2048];
    char baseFileName[512];
    
    // Extract base filename without extension if it ends with .c
    strncpy(baseFileName, fileName, sizeof(baseFileName) - 1);
    baseFileName[sizeof(baseFileName) - 1] = '\0';
    
    char* dotPosition = strrchr(baseFileName, '.');
    if (dotPosition && strcmp(dotPosition, ".c") == 0) {
        *dotPosition = '\0';  // Remove the .c extension
    }
    
    // Create source file path and instrumented file path
    snprintf(sourceFilePath, sizeof(sourceFilePath), "%s/%s.c", sourcePath, baseFileName);
    snprintf(tempFilePath, sizeof(tempFilePath), "%s/%s.instrumented.c", sourcePath, baseFileName);
    
    // Command to add #include for coverage header
    snprintf(command, sizeof(command), 
             "sed '1i\\#include \"../headers/coverage.h\"' %s > %s", 
             sourceFilePath, tempFilePath);
             
    if (system(command) != 0) {
        fprintf(stderr, "Failed to add coverage header to file\n");
        return -1;
    }
    
    // Insert initialization in main function only
    // This is more reliable than trying to instrument every function
    snprintf(command, sizeof(command), 
             "sed -i '/int main/,/{/s/{/{\\n    __coverage_init();/1' %s",
             tempFilePath);
             
    if (system(command) != 0) {
        fprintf(stderr, "Failed to instrument target file\n");
        return -1;
    }
    
    return 0;
}

// Prepare target with coverage instrumentation and compile it
int prepareTargetWithCoverage(const char* sourcePath, const char* fileName) {
    char baseFileName[512];
    char tempFilePath[1024];
    char outputPath[1024];
    char command[2048];
    
    // Extract base filename without extension if it ends with .c
    strncpy(baseFileName, fileName, sizeof(baseFileName) - 1);
    baseFileName[sizeof(baseFileName) - 1] = '\0';
    
    char* dotPosition = strrchr(baseFileName, '.');
    if (dotPosition && strcmp(dotPosition, ".c") == 0) {
        *dotPosition = '\0';  // Remove the .c extension
    }
    
    // Instrument the source file
    if (instrumentTargetFile(sourcePath, baseFileName) != 0) {
        return -1;
    }
    
    // Path to the instrumented file and output binary
    snprintf(tempFilePath, sizeof(tempFilePath), "%s/%s.instrumented.c", sourcePath, baseFileName);
    snprintf(outputPath, sizeof(outputPath), "%s/%s.cov", sourcePath, baseFileName);
    
    // Compile the instrumented file with fuzz.c to provide __VERIFIER_nondet_int
    snprintf(command, sizeof(command), 
             "gcc -g -Wall -I%s/../headers -o %s %s %s/../src/coverage.c %s/../src/fuzz.c", 
             sourcePath, outputPath, tempFilePath, sourcePath, sourcePath);
             
    if (system(command) != 0) {
        fprintf(stderr, "Failed to compile instrumented target\n");
        return -1;
    }
    
    return 0;
}