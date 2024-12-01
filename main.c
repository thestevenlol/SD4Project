#include <stdio.h>
#include <stdlib.h>

#include "headers/hash.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        return 1;
    }

    const char* file_path = argv[1];
    char* full_path = get_full_path(file_path);
    char* hash = generate_program_hash(file_path);

    if (!hash) {
        fprintf(stderr, "Failed to generate program hash\n");
        return 1;
    }

    char* creation_time = get_current_time();
    if (!creation_time) {
        fprintf(stderr, "Failed to get current time\n");
        free(hash);
        return 1;
    }

    // Free allocated memory
    free(hash);
    free(creation_time);
    free(full_path);
}