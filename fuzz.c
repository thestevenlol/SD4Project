#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "headers/signals.h"
#include "headers/fuzztype.h"

static unsigned int counter = 0;

int generateRandomNumber(const int min, const int max) {
    // Increment counter each call
    counter++;
    
    // Mix time and counter for more entropy
    unsigned int seed = time(NULL) ^ (counter << 16);
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        // Mix in nanoseconds
        seed ^= ts.tv_nsec;
    }
    
    // Use seed to generate number
    srand(seed);
    return min + rand() % (max - min + 1);
}

char *generateRandomString(int length) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'#,./;[]{}()=+-_!\"£$^&*\\|<>@";
    char *random_string = NULL;

    // Handle invalid length
    if (length < 0) {
        return NULL;
    }

    // Handle zero length
    if (length == 0) {
        random_string = malloc(22); // Length of backup string + 1 for null terminator.
        if (!random_string) return NULL;
        strcpy(random_string, "random string backup!");
        return random_string;
    }

    // Allocate memory
    random_string = malloc(length + 1);
    if (!random_string) {
        return NULL;
    }

    // Generate random string
    for (int n = 0; n < length; n++) {
        int key = rand() % (sizeof(charset) - 1);
        random_string[n] = charset[key];
    }
    random_string[length] = '\0';

    return random_string;
}

char *generateMutatedString(char *str, int length, int seed) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'#,./;[]{}()=+-_!\"£$^&*\\|<>@";
    char *mutated_string = NULL;

    // Input validation
    if (!str || length <= 0) {
        return NULL;
    }

    // Allocate memory
    mutated_string = malloc(length + 1);
    if (!mutated_string) {
        return NULL;
    }

    // Copy string safely
    strncpy(mutated_string, str, length);
    mutated_string[length] = '\0';

    // Set random seed
    unsigned int old_seed = rand();
    srand(seed);

    // Perform mutation
    int index = rand() % length;
    char new_char;
    do {
        int key = rand() % (sizeof(charset) - 1);
        new_char = charset[key];
    } while (new_char == str[index]);
    
    mutated_string[index] = new_char;

    // Restore original random state
    srand(old_seed);

    return mutated_string;
}

char *flipBitInString(char *string, int length) {
    char *flipped_string = NULL;

    // Input validation
    if (!string || length <= 0) {
        return NULL;
    }

    // Allocate memory
    flipped_string = malloc(length + 1);
    if (!flipped_string) {
        return NULL;
    }

    // Copy string safely
    strncpy(flipped_string, string, length);
    flipped_string[length] = '\0';

    // Flip a random bit
    int index = rand() % length;
    int bit_position = rand() % 8; // Choose random bit position 0-7
    flipped_string[index] ^= (1 << bit_position); // Flip random bit at position

    return flipped_string;
}

char *insertCharIntoString(char *string, int length, char character) {
    char *inserted_string = NULL;

    // Input validation
    if (!string || length <= 0) {
        return NULL;
    }

    // Allocate memory for new string (original + 1 char + null terminator)
    inserted_string = malloc(length + 2);
    if (!inserted_string) {
        return NULL;
    }

    // Generate random insertion point
    int index = rand() % (length + 1);  // Allow insertion at end

    // Copy first part
    if (index > 0) {
        memcpy(inserted_string, string, index);
    }

    // Insert new character
    inserted_string[index] = character;

    // Copy remainder of string
    if (index < length) {
        memcpy(inserted_string + index + 1, string + index, length - index);
    }

    // Null terminate
    inserted_string[length + 1] = '\0';

    return inserted_string;
}

char *removeCharFromString(char *string, int length) {
    char *removed_string = NULL;

    // Input validation
    if (!string || length <= 0) {
        return NULL;
    }

    // Allocate memory for new string (original - 1 char + null terminator)
    removed_string = malloc(length);
    if (!removed_string) {
        return NULL;
    }

    // Generate random removal point
    int index = rand() % length;

    // Copy first part before removal point
    if (index > 0) {
        memcpy(removed_string, string, index);
    }

    // Copy remainder after removal point
    if (index < length - 1) {
        memcpy(removed_string + index, 
               string + index + 1, 
               length - index - 1);
    }

    // Ensure null termination
    removed_string[length - 1] = '\0';

    return removed_string;
}

int __VERIFIER_nondet_int() {
    return generateRandomNumber(1,5);
}