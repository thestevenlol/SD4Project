#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "headers/signals.h"
#include "fuzz.c"

#define MAX_INPUT 100

void vuln_input(char *input) {
    char buffer[50];
    strcpy(buffer, input);
    printf("Input: %s\n", buffer);
}

int main()
{

    initSignalHandler();
    srand(time(NULL));

    while (1) {
        int length = rand() % MAX_INPUT + 1;
        char* str = generate_random_string(length);
        vuln_input(str);
    }

    return 0;
}