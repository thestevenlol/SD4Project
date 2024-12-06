#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "headers/io.h"
#include "headers/testcase.h"

#define PATH_MAX 4096

static int test_counter = 1;

/**
 * @brief Creates a test suite directory structure
 * 
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
 * Gets current time formatted as required
 * @return Allocated string with formatted time
 */
char* getCurrentTime() {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    
    // Allocate buffer for time string
    char* time_str = malloc(26); // Standard ctime length
    if (!time_str) return NULL;
    
    // Weekday names
    const char* wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    // Month names
    const char* mon[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    
    // Format: "Thu Oct 17 17:20:15 2024"
    snprintf(time_str, 26, "%s %s %d %02d:%02d:%02d %d",
             wday[tm_info->tm_wday],
             mon[tm_info->tm_mon],
             tm_info->tm_mday,
             tm_info->tm_hour,
             tm_info->tm_min, 
             tm_info->tm_sec,
                tm_info->tm_year + 1900);
                
    return time_str;
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
    char* creationTime = getCurrentTime();
    printf("Creation time: %s\n", creationTime);
    
    if (!creationTime) return 0;
    
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
    fprintf(file, "\t<creationtime>%s</creationtime>\n", creationTime);
    fprintf(file, "</test-metadata>\n");
    
    fclose(file);

    free(creationTime);
    return 1;
}

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
    printf("Test suite directory created\n");

    if (!createMetadataFile(fullPath, filename)) {
        fprintf(stderr, "Failed to create metadata file\n");
        return 1;
    }
    printf("Metadata file created\n");
}

/**
 * Ensures test suite directory structure exists
 * @param test_suite Name of test suite
 * @return 1 on success, 0 on failure
 */
static int ensureTestSuiteDirs(const char* testSuite) {
    // Create base test-suites directory if needed
    if (mkdir("test-suites", 0777) == -1 && errno != EEXIST) {
        fprintf(stderr, "Failed to create test-suites dir: %s\n", strerror(errno));
        return 0;
    }
    
    // Create specific test suite directory if needed
    char suitePath[PATH_MAX];
    snprintf(suitePath, PATH_MAX, "test-suites/%s-test-suite", testSuite);
    if (mkdir(suitePath, 0777) == -1 && errno != EEXIST) {
        fprintf(stderr, "Failed to create test suite dir: %s\n", strerror(errno));
        return 0;
    }
    
    return 1;
}

int createTestInputFile(const int* inputs, size_t num_inputs, const char* test_suite) {
    if (!inputs || !test_suite) return 0;
    
    // Ensure directories exist
    if (!ensureTestSuiteDirs(test_suite)) {
        return 0;
    }
    
    char filepath[PATH_MAX];
    snprintf(filepath, PATH_MAX, "test-suites/%s-test-suite/test_input-%d.xml", 
             test_suite, test_counter);
    
    FILE* file = fopen(filepath, "w");
    if (!file) {
        fprintf(stderr, "Failed to create file %s: %s\n", 
                filepath, strerror(errno));
        return 0;
    }
    
    fprintf(file, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
    fprintf(file, "<!DOCTYPE testcase PUBLIC \"+//IDN sosy-lab.org//DTD test-format testcase 1.1//EN\" \"https://sosy-lab.org/test-format/testcase-1.1.dtd\">\n");
    fprintf(file, "<testcase>\n");
    for (size_t i = 0; i < num_inputs; i++) {
        fprintf(file, "\t<input>%d</input>\n", inputs[i]);
    }
    fprintf(file, "</testcase>\n");
    
    fclose(file);
    test_counter++;
    return 1;
}