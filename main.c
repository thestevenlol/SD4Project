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

    // Generate the metadata.xml content
    printf("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
    printf("<!DOCTYPE test-metadata PUBLIC \"+//IDN sosy-lab.org//DTD test-format test-metadata 1.1//EN\" \"https://sosy-lab.org/test-format/test-metadata-1.1.dtd\">\n");
    printf("<test-metadata>\n");
    printf("\t<sourcecodelang>C</sourcecodelang>\n");
    printf("\t<producer>Fuzzer</producer>\n");
    printf("\t<specification>CHECK( init(main()), FQL(cover EDGES(@DECISIONEDGE)) )</specification>\n");
    printf("\t<programfile>%s</programfile>\n", file_path);
    printf("\t<programhash>%s</programhash>\n", hash);
    printf("\t<entryfunction>main</entryfunction>\n");
    printf("\t<architecture>32bit</architecture>\n");
    printf("\t<creationtime>%s</creationtime>\n", creation_time);
    printf("</test-metadata>\n");

    // Free allocated memory
    free(hash);
    free(creation_time);
    free(full_path);
}