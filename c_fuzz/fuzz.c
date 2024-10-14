#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "headers/signals.h"

char *generate_random_string(int length) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char *random_string = NULL;

    if (length) {
        random_string = malloc(length + 1);
        if (random_string) {
            for (int n = 0; n < length; n++) {
                int key = rand() % (int)(sizeof(charset) - 1);
                random_string[n] = charset[key];
            }
            random_string[length] = '\0';
        }
    }

    return random_string;
}