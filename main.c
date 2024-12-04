#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "headers/testcase.h"
#include "headers/io.h"
#include "headers/lex.h"
#include "fuzz.c"

#define BATCH_SIZE 1000000
#define N_TESTS 20

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        return 1;
    }
    
    char* filepath = argv[1];
    char* fullPath = getFullPath(filepath);
    char* base = strrchr(filepath, '/');
    char* filename = base ? base + 1 : filename;
    
    createTestSuiteAndMetadata(fullPath, filename);

    // Lexical analysis
    int seed = time(NULL);
    srand(seed);

    if (generateLexer() != ERR_SUCCESS) {
        return 1;
    }

    if (lexScanFile(fullPath) != ERR_SUCCESS) {
        return 1;
    }

    struct InputRange range = extractInputRange(OUTPUT_FILE);
    if (!range.valid) {
        return 1;
    }

    printf("Input range: [%d, %d]\n", range.min, range.max);

    // Free memory. not needed anymore
    free(fullPath);

    int counter = 0;
    int inputs[BATCH_SIZE];
    int batch_count = 0;
    char* hash = getHash(filename);

    for (int i = 0; i < BATCH_SIZE * N_TESTS; i++) {
        counter++;
        inputs[batch_count++] = generateRandomNumber(range.min, range.max);
        
        if (batch_count == BATCH_SIZE) {
            createTestInputFile(inputs, batch_count, filename);
            batch_count = 0;
        }
    }

    free(hash);
    return 0;
    
}