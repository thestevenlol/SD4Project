#include <limits.h>
#include <stdlib.h>


#include "headers/hash.h"
#include "headers/testcase.h"

/**
 * @brief Opens a file in append and read mode.
 * 
 * This function attempts to open the specified file in "a+" mode,
 * which allows for both reading and appending to the file.
 * If the file doesn't exist, it will be created.
 * 
 * @param filename The name/path of the file to be opened
 * @return FILE* A pointer to the opened file, or NULL if the operation fails
 * 
 * @note The caller is responsible for closing the file when done
 * @see fclose()
 */
FILE* open_file(const char* filename) {
    FILE* file = fopen(filename, "a+");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }
    return file;
}

/**
 * Writes a string to a file stream and flushes the buffer.
 * 
 * @param file   Pointer to the FILE stream to write to
 * @param string Pointer to the null-terminated string to write
 * 
 * @return 1 on success, 0 if:
 *         - file pointer is NULL
 *         - string pointer is NULL 
 *         - writing to file fails
 *         - flushing the file buffer fails
 */
int writeStringToFile(FILE* file, const char* string) {
    if (file == NULL || string == NULL) {
        return 0;
    }
    
    if (fputs(string, file) == EOF) {
        return 0;
    }
    
    if (fflush(file) != 0) {
        return 0;
    }
    
    return 1;
}

int createMetadataFile(const char* filename) {
    printf("Creating metadata file for: %s\n", filename);
    size_t needed = strlen(filename) + strlen("-test-suite/metadata.xml") + 1;
    char* testSuitName = (char*)malloc(needed);
    if (!testSuitName) return 0;
    
    snprintf(testSuitName, needed, "%s-test-suite", filename);
    if (!createTestSuiteFolder(testSuitName)) {
        free(testSuitName);
        return 0;
    }
    printf("Test suite folder created at: %s\n", testSuitName);
    
    snprintf(testSuitName, needed, "test-suites/%s-test-suite/metadata.xml", filename);
    printf("Metadata file path: %s\n", testSuitName);

    FILE* file = open_file(testSuitName);
    if (file == NULL) {
        return 0;
    }

    const char *xml_metadata = 
    "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
    "<!DOCTYPE test-metadata PUBLIC \"+//IDN sosy-lab.org//DTD test-format test-metadata 1.1//EN\" "
    "\"https://sosy-lab.org/test-format/test-metadata-1.1.dtd\">\n"
    "<test-metadata>\n"
    "\t<sourcecodelang>C</sourcecodelang>\n"
    "\t<producer>Fuzzer</producer>\n"
    "\t<specification>CHECK( init(main()), FQL(cover EDGES(@DECISIONEDGE)) )</specification>\n"
    "\t<programfile>%s</programfile>\n"
    "\t<programhash>%s</programhash>\n"
    "\t<entryfunction>main</entryfunction>\n"
    "\t<architecture>32bit</architecture>\n"
    "\t<creationtime>%s</creationtime>\n"
    "</test-metadata>\n";

    char formatted_metadata[1024]; // Adjust size as needed
    const char* hash = generate_program_hash(filename);
    const char* creation_time = get_current_time();
    const char* file_path = get_full_path(filename);
    snprintf(formatted_metadata, sizeof(formatted_metadata), xml_metadata, file_path, hash, creation_time);
    
    if (!writeStringToFile(file, formatted_metadata)) {
        close_file(file);
        return 0;
    }

    void* tmp_hash = (void*)hash;
    void* tmp_creation_time = (void*)creation_time;
    void* tmp_file_path = (void*)file_path;

    free(tmp_hash);
    free(tmp_creation_time);
    free(tmp_file_path);
    free(testSuitName);

    close_file(file);

    return 1;
}

int createTestSuiteFolder(const char* foldername) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("Error getting current working directory");
        return 0;
    }

    printf("Current working directory: %s\n", cwd);

    // First create test-suites directory if it doesn't exist
    char test_suites_path[PATH_MAX];
    snprintf(test_suites_path, sizeof(test_suites_path), "%s/test-suites", cwd);
    mkdir(test_suites_path, 0777); // Ignore error as directory might already exist

    // Create the specific test suite folder inside test-suites
    char fullpath[PATH_MAX];
    snprintf(fullpath, sizeof(fullpath), "%s/test-suites/%s", cwd, foldername);

    printf("Creating test suite folder: %s\n", fullpath);
    if (mkdir(fullpath, 0777) == -1) {
        perror("Error creating test suite folder");
        return 0;
    }
    return 1;
}

/**
 * @brief Closes a file stream safely
 * 
 * This function attempts to close a file stream and performs error checking.
 * It verifies if the file pointer is valid and ensures the file is closed properly.
 * 
 * @param file Pointer to the FILE stream to be closed
 * @return int Returns 1 on successful closure, 0 on failure or if file is NULL
 */
int close_file(FILE* file) {
    if (file == NULL) {
        return 0;
    }
    if (fclose(file) != 0) {
        return 0;
    }
    return 1;
}