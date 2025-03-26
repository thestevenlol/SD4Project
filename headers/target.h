// filepath: headers/target.h
#ifndef TARGET_H
#define TARGET_H

#include <limits.h> // For PATH_MAX (might need adjustment for portability)

// Compile the target program using Clang with coverage instrumentation.
// Links the coverage runtime.
// sourceDir: Directory containing the source file and coverage_runtime.c
// sourceFileName: Name of the target source file (e.g., "Problem10.c")
// outputExeName: Desired name for the instrumented executable (e.g., "problem10_fuzz")
int compile_target_with_clang_coverage(const char *sourceDir,
                                      const char *sourceFileName,
                                      const char *outputExeName);


// Execute the instrumented target in a controlled environment (fork/exec).
// Returns the exit status of the child process.
// Special return values might indicate timeout (-SIGALRM) or crash (signal number).
// exePath: Path to the compiled instrumented executable.
// input: The integer input to pass to the target (via stdin pipe).
// timeout_ms: Timeout in milliseconds for the target execution.
int execute_target_fork(const char *exePath, int input, unsigned int timeout_ms);


// Cleanup function (might remove compiled target)
void cleanup_target(const char *exePath);


// --- Removed/Deprecated ---
// int compileTargetFile(const char *sourcePath, const char *fileName); // Replaced
// int executeTargetInt(int input); // Replaced by execute_target_fork

#endif // TARGET_H