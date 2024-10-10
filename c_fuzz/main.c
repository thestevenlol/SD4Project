#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EQ_SIZE 256 * sizeof(char)

void solve_equation(char *equation) {
    // Recognize and solve simple equations of the form "x + b = c" or "x - b = c"
    char variable;
    int b, c;
    int result;
    char operator;

    if (sscanf(equation, "%c %c %d = %d", &variable, &operator, &b, &c) == 4) {
        if (operator == '+') {
            result = c - b;
        } else if (operator == '-') {
            result = c + b;
        } else {
            fprintf(stderr, "Unsupported operator: %c\n", operator);
            free(equation);
            return;
        }
        printf("The value of %c is: %d\n", variable, result);
    } else {
        fprintf(stderr, "Equation format not recognized.\n");
    }

    free(equation);
}

int main()
{

    printf("Welcome to the calculator! Please enter your equation below!\n");

    char *equation = malloc(EQ_SIZE);

    // Read the equation from the user
    if (fgets(equation, EQ_SIZE, stdin) == NULL) {
        fprintf(stderr, "Error reading input.\n");
        free(equation);
        return 1;
    }

    // Check if the input size exceeds EQ_SIZE
    if (equation[strlen(equation) - 1] != '\n' && !feof(stdin)) {
        fprintf(stderr, "Input exceeds maximum size of %d characters.\n", EQ_SIZE - 1);
        free(equation);
        return 1;
    }

    // Remove all whitespace from the equation
    char *src = equation, *dst = equation;
    while (*src) {
        if (*src != ' ' && *src != '\t' && *src != '\n' && *src != '\r') {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';

    // Extract variables from the equation
    char variables[256] = {0};
    int var_count = 0;

    for (char *ptr = equation; *ptr != '\0'; ptr++) {
        if ((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z')) {
            if (strchr(variables, *ptr) == NULL) {
                variables[var_count++] = *ptr;
            }
        }
    }

    // Print extracted variables
    if (var_count > 0) {
        printf("Extracted variables: ");
        for (int i = 0; i < var_count; i++) {
            printf("%c ", variables[i]);
        }
        printf("\n");
    } else {
        printf("No variables found in the equation.\n");
    }

    // Declare the external function to generate a random equation using a fuzzer
    extern char* generate_fuzzed_equation();

    // Use the fuzzer to generate the equation
    free(equation); // Free the previously allocated memory
    equation = generate_fuzzed_equation();

    solve_equation(equation);

    return 0;
}