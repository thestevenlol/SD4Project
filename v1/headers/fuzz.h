#ifndef FUZZ_H
#define FUZZ_H

#include "range.h"

// Random generation functions
int generateRandomNumber();
char* generateRandomString(int length);
char* generateMutatedString(char* str, int length, int seed);

// Bit-level and byte-level mutations
int mutateBitFlip(int value);
int mutateByteFlip(int value);
char* flipBitInString(char* string, int length);

// Block-based mutations
char* insertCharIntoString(char* string, int length, char character);
char* removeCharFromString(char* string, int length);

// Advanced mutation functions
// Mutate integer within specified range
int mutateInteger(int original_value, int min_range, int max_range);
int mutateArithmetic(int value); 
int mutateDictionary(int value);
int mutateHavoc(int value);  // Apply multiple random mutations

// Crossover functionality
int crossover(int parent1, int parent2);

// Other utility functions
int generateSequence(int length);
int __VERIFIER_nondet_int(void);

#endif