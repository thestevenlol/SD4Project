#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int input;
    // Read integer from stdin
    if (scanf("%d", &input) != 1) {
        fprintf(stderr, "Usage: %s <integer>\n", "Problem3");
        return 1;
    }

    printf("Received input: %d\n", input);

    // Vulnerability: stack buffer overflow when input > 32
    char buffer[32];
    if (input > 32) {
        printf("Triggering buffer overflow with %d bytes\n", input);
        // Write 'A' for input bytes (overflow when input > 32)
        for (int i = 0; i < input; i++) {
            buffer[i] = 'A';
        }
        buffer[31] = '\0';
        printf("Buffer content: %s\n", buffer);
    } else {
        printf("Input below threshold, safe execution.\n");
    }

    return 0;
}