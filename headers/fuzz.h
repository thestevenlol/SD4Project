#ifndef FUZZ_H
#define FUZZ_H

#include "range.h"

// Remove the extern declarations since they're now in range.h
int generateRandomNumber();
char* generateRandomString(int length);
char* generateMutatedString(char* str, int length, int seed);
char* flipBitInString(char* string, int length);
char* insertCharIntoString(char* string, int length, char character);
char* removeCharFromString(char* string, int length);
int __VERIFIER_nondet_int(void);

#endif