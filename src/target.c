#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include "../headers/target.h"
#include "../headers/range.h"

int compileTargetFile(const char* sourcePath, const char* fileName) {
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
    snprintf(copy_cmd, sizeof(copy_cmd), "cp \"%s\" coverage/Problem10.c", absolutePath);
    if (system(copy_cmd) != 0) {
        return 1;
    }

    // Create and write our test harness
    if (chdir("coverage") != 0) {
        return 1;
    }

    // Compile everything together with coverage
    int result = system("gcc -c --coverage Problem10.c -o Problem10.o && "
                       "gcc -o source Problem10.o ../src/fuzz.c ../src/range.c --coverage -lgcov");
    
    // Change back to original directory
    chdir("..");
    return result;
}

// Execute the compiled target with an input value
int executeTargetInt(int input) {
    // Save current directory
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        return -1;
    }

    // Change to coverage directory
    if (chdir("coverage") != 0) {
        return -1;
    }

    // Execute the program with both input value and range values
    char command[64];
    snprintf(command, sizeof(command), "./source %d %d %d", input, minRange, maxRange);
    printf("Executing: %s\n", command);
    
    // Run the program and generate coverage data
    int result = system(command);
    
    // Run gcov on the program
    system("gcov Problem10.c");

    // Return to original directory
    chdir(cwd);
    return result;
}

void cleanupTarget() {
    system("rm -rf coverage/source");
}