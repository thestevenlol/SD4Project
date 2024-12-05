#ifndef FUZZTYPE_H
#define FUZZTYPE_H

#define EXECUTABLE "temp_executable"

int generateRandomNumber();

char *generateRandomString(int length);

char *generateMutatedString(char *string, int length, int seed);

char *flipBitInString(char *string, int length);

char *insertCharIntoString(char *string, int length, char character);

char *removeCharFromString(char *string, int length);

int executeTargetInt(int input);

#endif