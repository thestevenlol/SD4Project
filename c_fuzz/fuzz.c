#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "headers/signals.h"

void generate_fuzzed_equation(char *buffer, size_t length) {
    initSignalHandler();

    const char charset[] = "0123456789+-*/";
    if (length) {
        --length;
        for (size_t i = 0; i < length; i++) {
            int key = rand() % (int)(sizeof(charset) - 1);
            buffer[i] = charset[key];
        }
        buffer[length] = '\0';
    }
}