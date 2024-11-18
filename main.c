#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "fuzz.c"
#include "headers/signals.h"

/**
 * Performs complex string and numeric operations based on flag bits
 * 
 * @param input      Input string to process
 * @param flag       Bit mask controlling which operations to perform:
 *                   0x01 - String processing: converts letters to position values (a=1,b=2...)
 *                         and doubles numeric digits
 *                   0x02 - Numeric manipulation: alternates between adding and subtracting
 *                         character values scaled by multiplier
 *                   0x04 - Pattern matching: doubles result if 3+ consecutive identical
 *                         characters are found
 * @param multiplier Scaling factor used in numeric manipulation (flag 0x02)
 * 
 * @return          -1 if input is NULL or empty
 *                  result % 100 if result < 0 or > 10000
 *                  strlen(input) if result == 0
 *                  calculated result otherwise
 * 
 * Example usage:
 *   complexOperation("abc123", 0x01, 1.0)  // Processes letters and numbers
 *   complexOperation("test", 0x02, 2.5)    // Performs numeric scaling
 *   complexOperation("aaa", 0x04, 1.0)     // Detects pattern and doubles result
 * 
 * Note: Multiple flags can be combined with bitwise OR:
 *   complexOperation("test", 0x03, 1.0)    // Performs both string and numeric processing
 */
int complexOperation(char* input, int flag, float multiplier) {
    int result = 0xFFFFFF;
    
    // Input validation
    if (!input || strlen(input) == 0) {
        return -1;
    }

    // Branch 1: String processing
    if (flag & 0x01) {
        int sum = 0;
        for (int i = 0; input[i] != '\0'; i++) {
            if (isalpha(input[i])) {
                sum += (input[i] & 0x1F);
            } else if (isdigit(input[i])) {
                sum += (input[i] - '0') * 2;
            }
        }
        result += sum;
    }

    // Branch 2: Numeric manipulation
    if (flag & 0x02) {
        float temp = 0;
        for (int i = 0; i < strlen(input); i++) {
            if (i % 2 == 0) {
                temp += (input[i] * multiplier);
            } else {
                temp -= (input[i] / multiplier);
            }
        }
        result += (int)temp;
    }

    // Branch 3: Pattern matching with bug
    if (flag & 0x04) {
        char small_buffer[8];  // Small buffer
        strcpy(small_buffer, input);  // Buffer overflow when input > 7 chars
        
        int consecutive = 0;
        size_t len = strlen(small_buffer);
        for (int i = 1; i < len; i++) { 
            if (small_buffer[i] == small_buffer[i-1]) {
                consecutive++;
                if (consecutive > 2) {
                    result *= 2;
                    break;
                }
            } else {
                consecutive = 0;
            }
        }
    }

    // Branch 4: Special conditions
    if (result < 0 || result > 10000) {
        return result % 100;
    } else if (result == 0) {
        return strlen(input);
    }

    return result;
}

int main(int argc, char **argv) {

    initSignalHandler();
    int iteration = 1;

    while (1)
    {
        int mask = (rand() % 4) + 1;
        int multiplier = (rand() % 10) + 1;
        int length = generateRandomNumber(1, 100);
        char *input = generateRandomString(length);


        int result = complexOperation(input, mask, multiplier);
        if (iteration % 10000 == 0) {
            printf("Iteration #%d\nMask: %d\nMultiplier: %d\nInput: %s\nResult: %d\n\n", iteration, mask, multiplier, input, result);
        }

        free(input);
        iteration++;
    }

    return 0;
}