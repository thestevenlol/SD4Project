#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// Function to generate a random number between 1 and 6
int __VERIFIER_nondet_int() {
    // Seed the random number generator
    srand(time(NULL));

    

    // Generate a random number between 1 and 6
    return (rand() % 6) + 1;
}

void exit(int status) {
    printf("Intercepted exit with status: %d\n", status);
}

void calculate_output(int input);

int main() {
    printf("Starting\n");
    for (size_t i = 0; i < 100; i++)
    {
        int input = __VERIFIER_nondet_int();
        printf("Generated input: %d\n", input);
        calculate_output(input);
    }
    
    printf("Done\n");
    return 0;
}