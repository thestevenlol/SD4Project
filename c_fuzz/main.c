#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "headers/signals.h"
#include "fuzz.c"

#define MAX_INPUT 215
#define BUFFER_SIZE 200

void vuln_input(char *input) {
    char buffer[BUFFER_SIZE];

    if (input == NULL) {
        printf("Error: input is NULL!\n");
        return;
    }

    strcpy(buffer, input);
    printf("Input: %s\n", buffer);
}

int main()
{

    initSignalHandler();
    srand(time(NULL));

    while (1) {
        int length = rand() % MAX_INPUT;
        char* str = generate_random_string(length);
        vuln_input(str);
        free(str);
    }

    return 0;
}