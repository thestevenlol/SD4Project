#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <libgen.h>
#include "headers/fuzz.h" 
#include "headers/lex.h"   
#include "headers/io.h"   
#include "headers/testcase.h"    
#include "headers/target.h" 
#include "headers/range.h"

#define BATCH_SIZE 100000
#define N_TESTS 20

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    char *temp_path = strdup(filename);
    const char *base_filename = basename(temp_path);

    char *fullPath = realpath(filename, NULL);
    if (!fullPath) {
        perror("Error getting full path");
        return 1;
    }

    printf("Full path: %s\n", fullPath);    
    printf("Filename: %s\n", filename);
    printf("Base filename: %s\n", base_filename);
    createTestSuiteAndMetadata(fullPath, base_filename);

    unsigned int seed = time(NULL);
    printf("Using seed: %u\n", seed);
    srand(seed);

    printf("Generating lexer...\n");
    if (generateLexer() != ERR_SUCCESS) {
        printf("Failed to generate lexer\n");
        return 1;
    }

    printf("Scanning file: %s\n", fullPath);
    if (lexScanFile(fullPath) != ERR_SUCCESS) {
        printf("Failed to scan file\n");
        return 1;
    }

    struct InputRange range = extractInputRange(OUTPUT_FILE);
    if (!range.valid) {
        printf("Failed to extract input range\n");
        return 1;
    }

    printf("Input range: [%d, %d]\n", range.min, range.max);
    minRange = range.min;
    maxRange = range.max;

    free(fullPath);

    int counter = 0;
    int input = 0;
    int inputs[BATCH_SIZE];
    int batch_count = 0;
    char* hash = getHash(filename);

    printf("Starting test generation...\n");
    for (int i = 0; i < BATCH_SIZE * N_TESTS; i++) {
        counter++;
        input = generateRandomNumber();
        inputs[batch_count++] = input;
        
        if (batch_count == BATCH_SIZE) {
            printf("Creating test batch %d, filename: %s\n", counter / BATCH_SIZE, filename);
            createTestInputFile(inputs, batch_count, base_filename);
            batch_count = 0;
        }
    }

    free(temp_path); // Free temporary buffer
    free(hash);
    printf("Test generation complete\n");
    return 0;
}