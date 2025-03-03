#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include "../headers/target.h"
#include "../headers/range.h"

int compileTargetFile(const char* sourcePath, const char* fileName) {
    printf("Compiling target file: %s\n", sourcePath);
    char absolutePath[PATH_MAX];
    if (realpath(sourcePath, absolutePath) == NULL) {
        return 1;
    }

    // Create coverage directory and set it up
    if (system("mkdir -p coverage") != 0) {
        return 1;
    }

    // Copy the target program to coverage directory
    char copy_cmd[PATH_MAX * 2];
    snprintf(copy_cmd, sizeof(copy_cmd), "cp \"%s\" coverage/Problem.c", absolutePath);
    if (system(copy_cmd) != 0) {
        return 1;
    }

    // Create and write our test harness
    if (chdir("coverage") != 0) {
        return 1;
    }

    // Compile everything together with coverage
    int result = system("gcc -c --coverage Problem.c -o Problem.o && "
                       "gcc -o source Problem.o ../src/fuzz.c ../src/generational.c ../src/range.c -g --coverage -lgcov");
    
    // Change back to original directory
    chdir("..");

    printf("Compilation result: %d\n", result);
    return result;
}

// Execute the compiled target with an input value
int executeTargetInt(int input) {
    // Save current directory
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("Failed to get current directory");
        return -1;
    }
    
    // Change to coverage directory
    if (chdir("coverage") != 0) {
        perror("Failed to change to coverage directory");
        return -1;
    }
    
    // Check if the executable exists
    if (access("./source", X_OK) != 0) {
        perror("Executable 'source' not found or not executable");
        chdir(cwd);
        return -1;
    }
    
    // Clean up existing gcda files before execution to reset coverage
    system("rm -f *.gcda");
    
    // Set the input value as an environment variable
    char env_var[32];
    snprintf(env_var, sizeof(env_var), "TEST_INPUT=%d", input);
    putenv(env_var);
    
    // Execute the program with input value
    char command[256];
    snprintf(command, sizeof(command), "./source");
    
    // Run the program and generate coverage data
    int result = system(command);
    if (result == -1) {
        perror("Failed to execute command");
        chdir(cwd);
        return -1;
    }
    
    // Run gcov on the program to generate fresh coverage data
    if (system("gcov -b -c Problem.c > /dev/null 2>&1") != 0) {
        fprintf(stderr, "Warning: gcov failed to generate coverage data\n");
    }
    
    // Return to original directory
    if (chdir(cwd) != 0) {
        perror("Failed to return to original directory");
        return -1;
    }
    
    return result;
}

void cleanupTarget() {
    system("rm -rf coverage/source");
}