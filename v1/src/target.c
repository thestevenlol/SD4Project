// filepath: src/target.c
#define _GNU_SOURCE // For pipe2, kill, readlink
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <fcntl.h> // For pipe2 flags, O_WRONLY, O_CLOEXEC
#include <errno.h>
#include <limits.h> // PATH_MAX

#include "../headers/target.h"
#include "../headers/coverage.h" // For fuzz_shared_mem, child_timed_out, reset_coverage_map

// Define specific error code for internal fuzzer execution errors
#define FUZZER_EXEC_ERROR -999

// Compile the target program using Clang with coverage instrumentation.
int compile_target_with_clang_coverage(const char *sourceDir,
    const char *sourceFileName,
    const char *outputExeName)
{
char command[PATH_MAX * 4];
char sourceFilePath[PATH_MAX];
char runtimeFilePath[PATH_MAX];
char outputFilePath[PATH_MAX];
char selfPath[PATH_MAX];
char *lastSlash;

// Construct full path to the target source file
snprintf(sourceFilePath, sizeof(sourceFilePath), "%s/%s", sourceDir, sourceFileName);

// --- Attempt to locate coverage_runtime.c ---
runtimeFilePath[0] = '\0';
if (readlink("/proc/self/exe", selfPath, sizeof(selfPath)-1) != -1) {
selfPath[sizeof(selfPath)-1] = '\0';
lastSlash = strrchr(selfPath, '/');
if (lastSlash) {
*lastSlash = '\0';
snprintf(runtimeFilePath, sizeof(runtimeFilePath), "%s/src/coverage_runtime.c", selfPath);
}
}
if (runtimeFilePath[0] == '\0' || access(runtimeFilePath, F_OK) != 0) {
snprintf(runtimeFilePath, sizeof(runtimeFilePath), "src/coverage_runtime.c");
}
if (access(runtimeFilePath, F_OK) != 0) {
fprintf(stderr,"Compile Error: Cannot find coverage_runtime.c\n");
return -1;
}

// Construct full path for the output executable
snprintf(outputFilePath, sizeof(outputFilePath), "%s/%s", sourceDir, outputExeName);

// --- Build the Clang command using trace-cmp ---
// Add -fsanitize-coverage=trace-cmp for compilation AND linking
// Remove -fsanitize=address for now
snprintf(command, sizeof(command),
"clang -g -fsanitize-coverage=trace-cmp -o \"%s\" \"%s\" \"%s\" -Wl,--no-as-needed -fsanitize-coverage=trace-cmp", // Use trace-cmp
outputFilePath,
sourceFilePath,
runtimeFilePath);

printf("Fuzzer Info: Compiling target with command:\n  %s\n", command);

// ... (system call and checking remain the same) ...
int result = system(command);
if (result == -1) {
perror("Fuzzer Error: system() call failed during compilation");
return -1;
} else if (WIFEXITED(result)) {
int exit_status = WEXITSTATUS(result);
if (exit_status != 0) {
fprintf(stderr, "Fuzzer Error: Target compilation failed (Clang exited with status %d)\n", exit_status);
return -1;
}
} else {
fprintf(stderr, "Fuzzer Error: Target compilation terminated abnormally (status: %d)\n", result);
return -1;
}

printf("Fuzzer Info: Target compiled successfully: %s\n", outputFilePath);
return 0;
}

// Execute the instrumented target in a controlled environment (fork/exec).
// Return codes:
//   0: Normal exit(0)
//  +N: Normal exit(N) where N > 0 (e.g., +1, +100 from child errors)
//  -S: Terminated by signal S (e.g., -6 for SIGABRT, -11 for SIGSEGV, -SIGALRM for timeout)
// FUZZER_EXEC_ERROR (-999): Internal fuzzer error during execution setup (fork/pipe/etc.)
int execute_target_fork(const char *exePath, int input, unsigned int timeout_ms) {
    int status = 0; // Final status to return
    pid_t child_pid;
    int pipe_stdin[2];
    int wait_status; // Raw status from waitpid

    // Pre-execution checks
    if (fuzz_shared_mem.shm_id < 0 || !fuzz_shared_mem.map) {
        fprintf(stderr, "[Exec] Error: Shared memory not initialized.\n"); // Use stderr
        return FUZZER_EXEC_ERROR;
    }
    reset_coverage_map(); // Ensure coverage map is clean before run

    // Create pipe for feeding input to child's stdin
    if (pipe2(pipe_stdin, O_CLOEXEC) < 0) { // O_CLOEXEC prevents fd leak on execv
        perror("[Exec] Error: pipe2(pipe_stdin) failed"); // Use stderr + perror
        return FUZZER_EXEC_ERROR;
    }

    // Fork the fuzzer process
    child_pid = fork();

    if (child_pid < 0) {
        // Fork failed
        perror("[Exec] Error: fork failed"); // Use stderr + perror
        close(pipe_stdin[0]); close(pipe_stdin[1]); // Clean up pipe fds
        return FUZZER_EXEC_ERROR;
    }

    // --- Child Process ---
    if (child_pid == 0) {
        // Close the write end of the stdin pipe (child only reads)
        close(pipe_stdin[1]);
        // Redirect child's stdin to read from the pipe's read end
        if (dup2(pipe_stdin[0], STDIN_FILENO) < 0) {
            perror("Child Error: dup2(stdin) failed"); exit(100); // Use distinct codes for child setup errors
        }
        // Close the original read end descriptor (no longer needed after dup2)
        close(pipe_stdin[0]);

        // Set environment variable for shared memory ID so runtime can attach
        char shm_env_var[64];
        snprintf(shm_env_var, sizeof(shm_env_var), "__AFL_SHM_ID=%d", fuzz_shared_mem.shm_id);
        if (putenv(shm_env_var) != 0) {
             perror("Child Error: putenv(__AFL_SHM_ID) failed"); exit(101);
        }

        // Redirect stdout/stderr to /dev/null to prevent target output clutter
        int dev_null = open("/dev/null", O_WRONLY);
        if (dev_null >= 0) {
            dup2(dev_null, STDOUT_FILENO);
            dup2(dev_null, STDERR_FILENO); // Redirect stderr as well
            close(dev_null);
        } else {
             perror("Child Warning: Could not open /dev/null");
        }

        // Prepare arguments for execv (program name, NULL terminator)
        char *const argv[] = {(char *)exePath, NULL};
        // Replace child process image with the target executable
        execv(exePath, argv);

        // If execv returns, it means an error occurred
        fprintf(stderr, "Child Error: execv failed: %s\n", strerror(errno));
        exit(102); // Exit with distinct code
    }

    // --- Parent Process (Fuzzer) ---
    close(pipe_stdin[0]); // Close read end (parent only writes)

    // Write input to child
    char input_str[32];
    snprintf(input_str, sizeof(input_str), "%d\n", input);
    ssize_t written = write(pipe_stdin[1], input_str, strlen(input_str));
    if (written <= 0 && errno != EPIPE) {
         fprintf(stderr, "[Exec] Warning: Failed to write full input to pipe: %s\n", strerror(errno));
    }
    close(pipe_stdin[1]); // Close pipe write end to signal EOF

    // Setup timer
    child_timed_out = 0;
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = fuzzer_signal_handler;
    sigaction(SIGALRM, &sa, NULL);

    unsigned int timeout_sec = timeout_ms / 1000;
    if (timeout_sec == 0 && timeout_ms > 0) timeout_sec = 1;
    alarm(timeout_sec);

    // Wait for child, handling EINTR
    do {
        if (waitpid(child_pid, &wait_status, 0) < 0) {
            if (errno == EINTR) {
                if (child_timed_out) {
                    fprintf(stderr, "[Exec] Timeout detected during EINTR loop (PID: %d)\n", child_pid);
                    kill(child_pid, SIGKILL);
                    waitpid(child_pid, &wait_status, 0);
                    status = -SIGALRM;
                    goto end_wait;
                }
                continue;
            } else {
                perror("[Exec] Error: waitpid failed (non-EINTR)");
                status = FUZZER_EXEC_ERROR;
                goto end_wait;
            }
        } else {
            break; // waitpid succeeded
        }
    } while (1);

end_wait:
    alarm(0); // Cancel alarm

    if (status != 0) { // Status already set by error/timeout in loop
         if (status == -SIGALRM) { // Ensure cleanup on timeout path
              kill(child_pid, SIGKILL);
              waitpid(child_pid, NULL, WNOHANG);
         }
         return status;
    }

    // Check timeout flag again after successful wait
    if (child_timed_out) {
        fprintf(stderr, "[Exec] Timeout detected after waitpid success (PID: %d)\n", child_pid);
        kill(child_pid, SIGKILL);
        waitpid(child_pid, NULL, WNOHANG);
        return -SIGALRM;
    }

    // Determine status from wait_status if waitpid succeeded and no timeout flag
    if (WIFEXITED(wait_status)) {
        int exit_code = WEXITSTATUS(wait_status);
        if (exit_code == 100 || exit_code == 101 || exit_code == 102) {
             fprintf(stderr, "[Exec] Warning: Child process setup error (exit code %d)\n", exit_code);
             return FUZZER_EXEC_ERROR;
        }
        return exit_code; // 0 or +N
    } else if (WIFSIGNALED(wait_status)) {
        int signal_num = WTERMSIG(wait_status);
        if (signal_num == SIGALRM || signal_num == SIGKILL) { // Should be caught by timeout flag, but handle defensively
             if (!child_timed_out) { fprintf(stderr, "[Exec] Info: Child killed by ALRM/KILL post-wait? (PID: %d)\n", child_pid); }
             return -SIGALRM;
        } else { // Genuine crash
             fprintf(stderr, "[Exec] Crash detected: Signal %d (PID: %d)\n", signal_num, child_pid);
             return -signal_num; // -S
        }
    }

    fprintf(stderr, "[Exec] Warning: Unknown child termination status: %d\n", wait_status);
    return FUZZER_EXEC_ERROR;
}

// Cleanup function (placeholder)
void cleanup_target(const char *exePath)
{
    if (exePath)
    {
        // Optional: remove(exePath);
        // printf("Fuzzer Info: Cleaned up target executable: %s\n", exePath);
    }
}