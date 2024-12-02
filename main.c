#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "headers/testcase.h"
#include "headers/io.h"
#include "headers/lex.h"

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

    int inputs[][4] = {
        {2, 9, 3, 3},
        {2, 4, 3, 3},
        {2, 4, 3, 3},
        {5, 8, 4, 6}
    };

    for (size_t i = 0; i < 4; i++)
    {
        if (!createTestInputFile(inputs[i], 4, filename)) {
            fprintf(stderr, "Failed to create test input file\n");
            return 1;
        }
    }

    int seed = time(NULL);
    srand(seed);

    if (generateLexer() != ERR_SUCCESS) {
        return 1;
    }

    if (lexScanFile("problems/Problem13.c") != ERR_SUCCESS) {
        return 1;
    }

    struct InputRange range = extractInputRange(OUTPUT_FILE);
    if (!range.valid) {
        return 1;
    }

    printf("Input range: [%d, %d]\n", range.min, range.max);

    free(fullPath);

    return 0;
    
}