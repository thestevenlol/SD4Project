#include <stdio.h>
#include <stdlib.h>

#include "headers/hash.h"
#include "headers/testcase.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        return 1;
    }
    
    char* filename = argv[1];
    char* base = strrchr(filename, '/');
    filename = base ? base + 1 : filename;
    if (!createMetadataFile(filename)) {
        return 1;
    }

    return 0;
    
}