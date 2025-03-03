#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include "../headers/signals.h"
#include "../headers/fuzz.h"
#include "../headers/generational.h"
#include "../headers/range.h"
#include "../headers/corpus.h"
#include "../headers/coverage.h"
#include "../headers/target.h"

static unsigned int counter = 0;

// Bit-level mutation implementation
int mutateBitFlip(int value) {
    // Choose a random bit to flip
    int bit_position = rand() % 32;
    return value ^ (1 << bit_position);
}

// Byte-level mutation implementation
int mutateByteFlip(int value) {
    // Choose a random byte to flip
    int byte_position = rand() % 4; // 4 bytes in an int
    int mask = 0xFF << (byte_position * 8);
    return value ^ mask;
}

// Arithmetic mutation implementation
int mutateArithmetic(int value) {
    // Choose an operation: add, subtract, multiply, divide
    int operation = rand() % 4;
    int amount = rand() % 100 + 1; // 1 to 100
    
    switch (operation) {
        case 0: // Add
            return value + amount;
        case 1: // Subtract
            return value - amount;
        case 2: // Multiply (small multiplier to avoid huge values)
            return value * (rand() % 5 + 1);
        case 3: // Divide (avoid division by zero)
            return value / (rand() % 10 + 1);
        default:
            return value;
    }
}

// Dictionary-based mutation
int mutateDictionary(int value) {
    // Array of "magic values" often used to find bugs
    static const int magic_values[] = {
        0, -1, 1, 
        INT_MAX, INT_MIN, 
        0x7FFFFFFF, 0x80000000, 
        0xFFFFFFFF,
        0x41414141, // "AAAA"
        0xDEADBEEF,
        0xC0FFEE,
        0
    };
    
    // Randomly replace with a magic value, or OR with it
    int magic = magic_values[rand() % (sizeof(magic_values) / sizeof(magic_values[0]))];
    
    // 50% chance to replace, 50% chance to OR
    if (rand() % 2) {
        return magic;
    } else {
        return value | magic;
    }
}

// Havoc mutation - apply multiple random mutations
int mutateHavoc(int value) {
    int mutations = rand() % 5 + 1; // Apply 1-5 random mutations
    int result = value;
    
    for (int i = 0; i < mutations; i++) {
        switch (rand() % 4) {
            case 0:
                result = mutateBitFlip(result);
                break;
            case 1:
                result = mutateByteFlip(result);
                break;
            case 2:
                result = mutateArithmetic(result);
                break;
            case 3:
                result = mutateDictionary(result);
                break;
        }
    }
    
    return result;
}

// Enhanced mutation with multiple strategies
int mutateInteger(int original_value, int min_range, int max_range) {
    // Choose a mutation strategy
    int strategy = rand() % 5;
    int mutated_value;
    
    switch (strategy) {
        case 0:
            mutated_value = mutateBitFlip(original_value);
            break;
        case 1:
            mutated_value = mutateByteFlip(original_value);
            break;
        case 2:
            mutated_value = mutateArithmetic(original_value);
            break;
        case 3:
            mutated_value = mutateDictionary(original_value);
            break;
        case 4:
            mutated_value = mutateHavoc(original_value);
            break;
        default:
            mutated_value = original_value;
    }
    
    // Ensure the value is within range (optional, can remove if you want to explore out-of-range values)
    if (mutated_value < min_range) {
        mutated_value = min_range;
    }
    if (mutated_value > max_range) {
        mutated_value = max_range;
    }
    
    return mutated_value;
}

// Crossover implementation
int crossover(int parent1, int parent2) {
    // Choose crossover strategy
    int strategy = rand() % 3;
    int child;
    
    switch(strategy) {
        case 0: // Single-point crossover
            {
                int crossover_point = rand() % 32; // For a 32-bit integer
                unsigned int mask = (1 << crossover_point) - 1;
                child = (parent1 & mask) | (parent2 & ~mask);
            }
            break;
            
        case 1: // Two-point crossover
            {
                int point1 = rand() % 32;
                int point2 = rand() % 32;
                if (point1 > point2) { 
                    int temp = point1;
                    point1 = point2;
                    point2 = temp;
                }
                unsigned int mask1 = (1 << point1) - 1;
                unsigned int mask2 = (1 << point2) - 1;
                unsigned int mask = mask2 ^ mask1;
                child = (parent1 & mask) | (parent2 & ~mask);
            }
            break;
            
        case 2: // Uniform crossover
            {
                child = 0;
                for (int i = 0; i < 32; i++) {
                    // For each bit position, randomly choose from which parent to take the bit
                    unsigned int bit_mask = 1 << i;
                    if (rand() % 2) {
                        // Take bit from parent1
                        child |= (parent1 & bit_mask);
                    } else {
                        // Take bit from parent2
                        child |= (parent2 & bit_mask);
                    }
                }
            }
            break;
            
        default:
            child = parent1; // Fallback
    }
    
    return child;
}

int generateSequence(int length) {
    int sum = 0;
    for (int i = 0; i < length; i++) {
        sum += generateRandomNumber();
        sum *= 10;
    }
    return sum;
}

int generateRandomNumber() {
    int minRange = 0;
    int maxRange = 9;
    
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
    int range = maxRange - minRange + 1;
    int result = minRange + (rand() % range);
    return result;
}

char *generateRandomString(int length)
{
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'#,./;[]{}()=+-_!\"£$^&*\\|<>@";
    char *random_string = NULL;
    
    // Handle invalid length
    if (length < 0)
    {
        return NULL;
    }
    
    // Handle zero length
    if (length == 0)
    {
        random_string = malloc(22); // Length of backup string + 1 for null terminator.
        if (!random_string)
            return NULL;
        strcpy(random_string, "random string backup!");
        return random_string;
    }
    
    // Allocate memory
    random_string = malloc(length + 1);
    if (!random_string)
    {
        return NULL;
    }
    
    // Generate random string
    for (int n = 0; n < length; n++)
    {
        int key = rand() % (sizeof(charset) - 1);
        random_string[n] = charset[key];
    }
    random_string[length] = '\0';
    return random_string;
}

char *generateMutatedString(char *str, int length, int seed)
{
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'#,./;[]{}()=+-_!\"£$^&*\\|<>@";
    char *mutated_string = NULL;
    
    // Input validation
    if (!str || length <= 0)
    {
        return NULL;
    }
    
    // Allocate memory
    mutated_string = malloc(length + 1);
    if (!mutated_string)
    {
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
    do
    {
        int key = rand() % (sizeof(charset) - 1);
        new_char = charset[key];
    } while (new_char == str[index]);
    mutated_string[index] = new_char;
    
    // Restore original random state
    srand(old_seed);
    return mutated_string;
}

char *flipBitInString(char *string, int length)
{
    char *flipped_string = NULL;
    
    // Input validation
    if (!string || length <= 0)
    {
        return NULL;
    }
    
    // Allocate memory
    flipped_string = malloc(length + 1);
    if (!flipped_string)
    {
        return NULL;
    }
    
    // Copy string safely
    strncpy(flipped_string, string, length);
    flipped_string[length] = '\0';
    
    // Flip a random bit
    int index = rand() % length;
    int bit_position = rand() % 8;                // Choose random bit position 0-7
    flipped_string[index] ^= (1 << bit_position); // Flip random bit at position
    return flipped_string;
}

char *insertCharIntoString(char *string, int length, char character)
{
    char *inserted_string = NULL;
    
    // Input validation
    if (!string || length <= 0)
    {
        return NULL;
    }
    
    // Allocate memory for new string (original + 1 char + null terminator)
    inserted_string = malloc(length + 2);
    if (!inserted_string)
    {
        return NULL;
    }
    
    // Generate random insertion point
    int index = rand() % (length + 1); // Allow insertion at end
    
    // Copy first part
    if (index > 0)
    {
        memcpy(inserted_string, string, index);
    }
    
    // Insert new character
    inserted_string[index] = character;
    
    // Copy remainder of string
    if (index < length)
    {
        memcpy(inserted_string + index + 1, string + index, length - index);
    }
    
    // Null terminate
    inserted_string[length + 1] = '\0';
    return inserted_string;
}

char *removeCharFromString(char *string, int length)
{
    char *removed_string = NULL;
    
    // Input validation
    if (!string || length <= 0)
    {
        return NULL;
    }
    
    // Allocate memory for new string (original - 1 char + null terminator)
    removed_string = malloc(length);
    if (!removed_string)
    {
        return NULL;
    }
    
    // Generate random removal point
    int index = rand() % length;
    
    // Copy first part before removal point
    if (index > 0)
    {
        memcpy(removed_string, string, index);
    }
    
    // Copy remainder after removal point
    if (index < length - 1)
    {
        memcpy(removed_string + index,
               string + index + 1,
               length - index - 1);
    }
    
    // Ensure null termination
    removed_string[length - 1] = '\0';
    return removed_string;
}

int __VERIFIER_nondet_int()
{
    return generateRandomNumber();
}