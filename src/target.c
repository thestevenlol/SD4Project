#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include "headers/target.h"

int compileTargetFile(const char* sourcePath, const char* fileName) {
    char absolutePath[PATH_MAX];
    if (realpath(sourcePath, absolutePath) == NULL) {
        return 1;
    }

    // Create coverage directory and set it up
    if (system("mkdir -p coverage") != 0) {
        return 1;
    }

    // Copy source file to coverage directory
    char copy_cmd[PATH_MAX * 2];
    snprintf(copy_cmd, sizeof(copy_cmd), "cp \"%s\" coverage/source.c", absolutePath);
    if (system(copy_cmd) != 0) {
        return 1;
    }

    // Change to coverage directory for compilation
    if (chdir("coverage") != 0) {
        return 1;
    }

    // Compile support files
    if (system("gcc -c -o fuzz.o ../src/fuzz.c -I.. && "
               "gcc -c -o range.o ../src/range.c -I..") != 0) {
        chdir("..");
        return 1;
    }

    // Compile and link with coverage
    int result = system("gcc -o source source.c fuzz.o range.o --coverage");
    
    // Change back to original directory
    chdir("..");
    return result;
}

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

    // Execute the program
    char command[32];
    snprintf(command, sizeof(command), "./source %d", input);
    printf("Executing: %s\n", command);
    int result = system(command);

    // Return to original directory
    chdir(cwd);
    return result;
}

void cleanupTarget() {
    system("rm -rf coverage/source");
}