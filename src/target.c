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
        perror("Failed to resolve path");
        return 1;
    }

    // Create coverage directory and set it up
    if (system("mkdir -p coverage") != 0) {
        perror("Failed to create coverage directory");
        return 1;
    }

    // Copy the target program to coverage directory
    char copy_cmd[PATH_MAX * 2];
    snprintf(copy_cmd, sizeof(copy_cmd), "cp \"%s\" coverage/Problem.c", absolutePath);
    if (system(copy_cmd) != 0) {
        perror("Failed to copy source file to coverage directory");
        return 1;
    }

    // Create and write our test harness
    if (chdir("coverage") != 0) {
        perror("Failed to change to coverage directory");
        return 1;
    }

    printf("Attempting to compile with coverage instrumentation...\n");
    
    // First attempt: Compile everything together with coverage
    int result = system("gcc -c --coverage Problem.c -o Problem.o && "
                       "gcc -o source Problem.o ../src/fuzz.c ../src/generational.c ../src/range.c -g --coverage -lgcov 2>&1");
    
    // If first compilation failed, try a simpler compilation without coverage
    if (result != 0) {
        printf("Coverage-enabled compilation failed. Trying simpler compilation...\n");
        result = system("gcc -o source Problem.c ../src/fuzz.c ../src/generational.c ../src/range.c -g 2>&1");
        
        if (result != 0) {
            // If that also fails, try with minimal dependencies
            printf("Second compilation attempt failed. Trying minimal compilation...\n");
            result = system("gcc -o source Problem.c ../src/fuzz.c -g 2>&1");
        }
    }
    
    if (result != 0) {
        fprintf(stderr, "All compilation attempts failed. Cannot create 'source' executable.\n");
        // Let's output the content of Problem.c to help diagnose the issue
        system("cat Problem.c | head -n 20");
    } else {
        printf("Compilation successful!\n");
        
        // Set executable permissions on the compiled binary
        if (system("chmod +x source") != 0) {
            fprintf(stderr, "Warning: Failed to set executable permissions\n");
        }
        
        // Verify the file exists
        if (access("./source", F_OK) != 0) {
            perror("Warning: 'source' file does not exist despite successful compilation");
            result = 1;  // Mark as failed
        } else {
            printf("Verified 'source' executable exists\n");
        }
    }
    
    // Change back to original directory
    chdir("..");
    printf("Compilation result: %d (0 means success)\n", result);
    return result;
}

// Execute the compiled target with an input value
int executeTargetInt(int input, char* filename) {
    // Save current directory
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("Failed to get current directory");
        return -1;
    }
    
    // Change to coverage directory
    if (chdir("coverage") != 0) {
        perror("Failed to change to coverage directory");
        // Try to create the coverage directory if it doesn't exist
        if (system("mkdir -p coverage") == 0) {
            printf("Created missing coverage directory\n");
            if (chdir("coverage") != 0) {
                perror("Still failed to change to coverage directory");
                return -1;
            }
        } else {
            perror("Failed to create coverage directory");
            return -1;
        }
    }
    
    // Check if the executable exists
    if (access("./source", X_OK) != 0) {
        perror("Executable 'source' not found or not executable");
        
        // Check if Problem.c exists in coverage directory
        if (access("Problem.c", F_OK) != 0) {
            fprintf(stderr, "Problem.c not found in coverage directory\n");
            
            // Go back to original directory to find the source file
            chdir(cwd);
            
            // If we can't find Problem.c, we need to find what Problem file was specified
            // Look for last used problem file in problems directory
            char cmd[PATH_MAX];
            snprintf(cmd, sizeof(cmd), "find %s/problems -name 'Problem*.c' -type f | sort | head -1", cwd);
            
            FILE *fp = popen(cmd, "r");
            if (fp == NULL) {
                perror("Failed to execute find command");
                return -1;
            }
            
            char sourcePath[PATH_MAX];
            if (fgets(sourcePath, sizeof(sourcePath), fp) == NULL) {
                perror("No Problem*.c files found in problems directory");
                pclose(fp);
                return -1;
            }
            pclose(fp);
            
            // Remove newline character
            size_t len = strlen(sourcePath);
            if (len > 0 && sourcePath[len-1] == '\n') {
                sourcePath[len-1] = '\0';
            }
            
            printf("Found source file: %s\n", sourcePath);
            
            // Extract filename for compileTargetFile
            char *fileName = strrchr(sourcePath, '/');
            if (fileName != NULL) {
                fileName++; // Move past the slash
            } else {
                fileName = sourcePath; // No directory part
            }
            
            printf("Recompiling with file: %s\n", fileName);
            
            // Use our existing compileTargetFile function to do the work properly
            if (compileTargetFile(sourcePath, fileName) != 0) {
                fprintf(stderr, "Failed to recompile target file\n");
                return -1;
            }
            
            // Change back to coverage directory 
            if (chdir("coverage") != 0) {
                perror("Failed to change back to coverage directory after recompile");
                return -1;
            }
            
            // Check if the executable exists now
            if (access("./source", X_OK) != 0) {
                perror("Executable 'source' still not found after recompilation");
                chdir(cwd);
                return -1;
            }
            
            printf("Successfully recompiled source executable\n");
        } else {
            // Problem.c exists but source is missing, just recompile it
            printf("Problem.c found but source missing, recompiling...\n");
            
            char compile_cmd[PATH_MAX * 2];
            snprintf(compile_cmd, sizeof(compile_cmd), 
                     "gcc -c --coverage %s -o Problem.o && "
                     "gcc -o source Problem.o ../src/fuzz.c ../src/generational.c ../src/range.c ../src/coverage.c -g --coverage -lgcov", 
                     filename);
            int result = system(compile_cmd);
            
            if (result != 0) {
                printf("Coverage-enabled compilation failed. Trying simpler compilation...\n");
                result = system("gcc -o source Problem.c ../src/fuzz.c ../src/generational.c ../src/range.c -g");
                
                if (result != 0) {
                    fprintf(stderr, "Failed to recompile source executable\n");
                    chdir(cwd);
                    return -1;
                }
            }
            
            // Set executable permissions
            system("chmod +x source");
            
            if (access("./source", X_OK) != 0) {
                perror("Executable 'source' still not found after recompilation");
                chdir(cwd);
                return -1;
            }
        }
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