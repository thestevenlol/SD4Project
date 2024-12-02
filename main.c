#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "headers/testcase.h"
#include "headers/io.h"

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

    free(fullPath);

    return 0;
    
}