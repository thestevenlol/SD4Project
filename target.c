#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "headers/target.h"

int compileTargetFile(const char* sourcePath, const char* fileName) {
    char fileNameNoExt[256];
    strncpy(fileNameNoExt, fileName, sizeof(fileNameNoExt));
    char* dot = strrchr(fileNameNoExt, '.');
    if (dot != NULL) {
        *dot = '\0';
    }
    
    char command[256];
    snprintf(command, sizeof(command), 
             "gcc --coverage -o \"%s\" \"%s\"", fileName, sourcePath);
    
    printf("Compiling with coverage: %s\n", command);
    return system(command);
}

// Execute the compiled target with an input value
int executeTargetInt(int input) {
    char command[256];
    int status;
    pid_t pid;
    
    snprintf(command, sizeof(command), 
             "./temp_executable %d", input);
    
    printf("Executing: %s\n", command);
    
    pid = fork();
    if (pid == 0) {
        // Child process
        execlp("./temp_executable", "temp_executable", 
               command, NULL);
        exit(1); // Only reached if exec fails
    } else if (pid > 0) {
        // Parent process
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        return -1;
    }
    
    return -1; // Fork failed
}

// Clean up compiled executable
void cleanupTarget() {
    unlink("temp_executable");
}