#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#include "headers/io.h"
#include "headers/testcase.h"

#define PATH_MAX 4096

/**
 * Creates a test suite directory and its associated metadata file.
 * 
 * @param fullPath The complete path to the source file
 * @param filename The name of the file for which the test suite is being created
 * @return int Returns 0 on success, 1 if either the directory or metadata file creation fails
 *
 * This function performs two main operations:
 * 1. Creates a test suite directory using the provided filename
 * 2. Generates a metadata file within that directory containing test suite information
 */
int createTestSuiteAndMetadata(const char* fullPath, const char* filename) {
    if (!createTestSuiteFolder(filename)) {
        fprintf(stderr, "Failed to create test suite directory\n");
        return 1;
    }

    if (!createMetadataFile(fullPath, filename)) {
        fprintf(stderr, "Failed to create metadata file\n");
        return 1;
    }
}

/**
 * Creates a test suite directory structure
 * @param filename Base filename used to name the test suite
 * @return 1 on success, 0 on failure
 */
int createTestSuiteFolder(const char* filename) {
    char path[PATH_MAX];
    
    // Create base test-suites directory
    if (snprintf(path, PATH_MAX, "test-suites") >= PATH_MAX) {
        return 0;
    }
    
    if (mkdir(path, 0777) == -1 && errno != EEXIST) {
        return 0;
    }
    
    // Create specific test suite directory
    if (snprintf(path, PATH_MAX, "test-suites/%s-test-suite", filename) >= PATH_MAX) {
        return 0;
    }
    
    if (mkdir(path, 0777) == -1 && errno != EEXIST) {
        return 0;
    }
    
    return 1;
}

/**
 * @brief 
 * 
 * Creates metadata.xml file in test suite directory
 * @param filename Base filename of the test suite
 * @return 1 on success, 0 on failure
 */
int createMetadataFile(const char* filePath, const char* filename) {
    char path[PATH_MAX];
    
    if (snprintf(path, PATH_MAX, "test-suites/%s-test-suite/metadata.xml", filename) >= PATH_MAX) {
        return 0;
    }
    
    FILE* file = fopen(path, "w");
    if (!file) {
        return 0;
    }
    
    // Write metadata content
    fprintf(file, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
    fprintf(file, "<!DOCTYPE test-metadata PUBLIC \"+//IDN sosy-lab.org//DTD test-format test-metadata 1.1//EN\" \"https://sosy-lab.org/test-format/test-metadata-1.1.dtd\">\n");
    fprintf(file, "<test-metadata>\n");
    fprintf(file, "\t<sourcecodelang>C</sourcecodelang>\n");
    fprintf(file, "\t<producer>Fuzzer</producer>\n");
    fprintf(file, "\t<specification>CHECK( init(main()), FQL(cover EDGES(@DECISIONEDGE)) )</specification>\n");
    fprintf(file, "\t<programfile>%s</programfile>\n", filePath);
    fprintf(file, "\t<programhash>%s</programhash>\n", getHash(filePath));
    fprintf(file, "\t<entryfunction>main</entryfunction>\n");
    fprintf(file, "\t<architecture>32bit</architecture>\n");
    fprintf(file, "</test-metadata>\n");
    
    fclose(file);
    return 1;
}