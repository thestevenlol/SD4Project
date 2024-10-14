#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "headers/signals.h"

char *generate_random_string(int length) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'#,./;[]{}()=+-_!\"£$\%^&*\\|<>@";
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

char *generate_mutated_string(char *string, int length) {
    char *mutated_string = NULL;

    if (string && length) {
        mutated_string = malloc(length + 1);
        if (mutated_string) {
            strcpy(mutated_string, string);
            int index = rand() % length;
            char new_char;
            do {
                new_char = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"[rand() % 62];
            } while (new_char == string[index]);
            mutated_string[index] = new_char;
        }
    }

    return mutated_string;
}