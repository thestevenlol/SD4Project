# C Fuzzer - 2024 Software Development

## C00274246 - Jack Foley

## What is this project?

This project is a fuzzing tool written in the C Programming language. It uses the standard C library on UNIX, glib.

## How to use

To use the fuzzer tools, you **must** add `fuzz.c` and **all header files** into your project. You will then have access to the following fuzzing functions.

```C
char *generateRandomString(int length);
char *generateMutatedString(char *str, int length, int seed);
char *flipBitInString(char *string, int length);
char *insertCharIntoString(char *string, int length, char character);
char *removeCharFromString(char *string, int length);
int generateRandomNumber(const int min, const int max);
```

### Example usage

```C
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include "fuzz.c"

// Function vulnerable to integer overflow
int add_numbers(int a, int b) {
    int result = a + b;
    return result;
}

int main() {
    // fuzzing loop, can be fixed or infinite.
    int iteration = 1;
    while (1) {
        int a = generateRandomNumber(INT_MAX - 100, INT_MAX);
        int b = generateRandomNumber(1, INT_MAX);
        int result = add_numbers(a, b);
        if (iteration % 100000 == 0) {
            printf("Iteration #%d\nResult: %d\n\n", iteration, result);
        }
        iteration++;
    }
}
```

It is vitally important to compile your code with the appropriate sanitizers. You should compile with the following command, where target.c is the name of your file(s): 

```shell
gcc -D_POSIX_C_SOURCE=199309L -g -rdynamic -fsanitize=undefined,address -fno-sanitize-recover -O1 -o output.o target.c
```

> If you remove the `-fno-sanitize-recover` flag the program will keep running after finding an error.

Then you can run in terminal with:

```shell
./output.o
```