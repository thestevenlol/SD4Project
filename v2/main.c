#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <errno.h>

// Coverage tracking constants
#define COVERAGE_MAP_SIZE (1 << 16)  // 64KB coverage map
typedef uint8_t coverage_t;

// Global variables for coverage tracking
coverage_t* __coverage_map;
uint32_t* __start_guards;
uint32_t* __stop_guards;
uint32_t __guard_count;

// Shared memory for coverage map
static int shm_id = -1;
static void* shared_mem_ptr = (void*)-1;
static pid_t child_pid = -1;
static volatile sig_atomic_t timed_out = 0;

// Fuzzer settings
#define MAX_ITERATIONS 10000
#define CORPUS_DIR "corpus"
#define CRASH_DIR "crashes"
#define TIMEOUT_DIR "timeouts"
#define TARGET_TIMEOUT_MS 1000

// Sanitizer callback functions for coverage instrumentation
void __sanitizer_cov_trace_pc_guard_init(uint32_t *start, uint32_t *stop) {
    // Called at program startup
    __start_guards = start;
    __stop_guards = stop;
    __guard_count = stop - start;
    
    printf("[INIT] Coverage guards: %u\n", __guard_count);
    
    // Initialize the coverage map
    __coverage_map = (coverage_t*)calloc(COVERAGE_MAP_SIZE, sizeof(coverage_t));
    if (!__coverage_map) {
        fprintf(stderr, "Failed to allocate coverage map\n");
        exit(1);
    }
}

void __sanitizer_cov_trace_pc_guard(uint32_t *guard) {
    // This is called for every edge in the instrumented code
    if (!__coverage_map) return;

    if (!guard) 
    {
        return; // Duplicate the guard check.
    }
    
    
    // Get the guard index and increment the corresponding counter
    uint32_t guard_index = guard - __start_guards;
    
    // Map to coverage map using simple modulo hash
    uint32_t map_index = guard_index % COVERAGE_MAP_SIZE;
    
    // Increment counter (typically with saturation at 255)
    if (__coverage_map[map_index] < 255) {
        __coverage_map[map_index]++;
    }
}

// Utility functions
void reset_coverage() {
    if (__coverage_map) {
        memset(__coverage_map, 0, COVERAGE_MAP_SIZE * sizeof(coverage_t));
    }
}

int count_covered_edges() {
    int count = 0;
    for (int i = 0; i < COVERAGE_MAP_SIZE; i++) {
        if (__coverage_map[i] > 0) {
            count++;
        }
    }
    return count;
}

// Save a test case to disk
void save_testcase(const char* dir, const void* data, size_t size, const char* suffix) {
    struct stat st = {0};
    
    // Create directory if it doesn't exist
    if (stat(dir, &st) == -1) {
        if (mkdir(dir, 0755) == -1 && errno != EEXIST) {
            fprintf(stderr, "Failed to create directory %s: %s\n", dir, strerror(errno));
            return;
        }
    }
    
    // Generate a filename based on time and optional suffix
    char filename[256];
    snprintf(filename, sizeof(filename), "%s/testcase_%ld_%s", 
             dir, (long)time(NULL), suffix ? suffix : "");
    
    // Write the test case to disk
    FILE* f = fopen(filename, "wb");
    if (f) {
        if (fwrite(data, 1, size, f) != size) {
            fprintf(stderr, "Failed to write testcase to %s\n", filename);
        } else {
            printf("Saved testcase to %s\n", filename);
        }
        fclose(f);
    } else {
        fprintf(stderr, "Failed to open %s for writing: %s\n", 
                filename, strerror(errno));
    }
}

// Simple mutation function
void mutate_buffer(uint8_t* buffer, size_t size) {
    if (size == 0) return;
    
    // Perform a simple mutation - in a real fuzzer you'd have multiple strategies
    int mutation_point = rand() % size;
    buffer[mutation_point] = rand() % 256;
}

// Main fuzzing loop
void run_fuzzer(const char* target_path, int iterations) {
    printf("Starting coverage-guided fuzzer...\n");
    
    // Initialize corpus with a basic test case
    uint8_t initial_input[] = "FUZZ";
    size_t initial_size = 4;
    
    // Create corpus directory
    struct stat st = {0};
    if (stat(CORPUS_DIR, &st) == -1) {
        mkdir(CORPUS_DIR, 0755);
    }
    
    // Save initial input to corpus
    save_testcase(CORPUS_DIR, initial_input, initial_size, "initial");
    
    // Statistics
    int total_executions = 0;
    int crashes = 0;
    int timeouts = 0;
    int new_coverage_found = 0;
    
    // Main fuzzing loop
    for (int i = 0; i < iterations && total_executions < MAX_ITERATIONS; i++) {
        // In a real implementation, you would select inputs from the corpus
        // instead of always using the initial one
        uint8_t current_input[1024];
        size_t current_size = initial_size;
        
        // Copy the initial input
        memcpy(current_input, initial_input, initial_size);
        
        // Mutate it
        mutate_buffer(current_input, current_size);
        
        // Execute the target with this input
        int result = execute_target(target_path, current_input, current_size);
        total_executions++;
        
        // Process results
        if (result < 0) {
            // A crash was detected
            crashes++;
            save_testcase(CRASH_DIR, current_input, current_size, "crash");
            printf("Crash detected! Total crashes: %d\n", crashes);
        } else if (result > 0) {
            // Non-zero exit code - might indicate a problem
            timeouts++;
            save_testcase(TIMEOUT_DIR, current_input, current_size, "timeout");
        }
        
        // Check if we found new coverage
        int covered = count_covered_edges();
        
        // In a real fuzzer, you would compare with previous coverage
        // and only save inputs that increase coverage
        // For simplicity, let's just save some random ones
        if (rand() % 100 < 5) {
            new_coverage_found++;
            save_testcase(CORPUS_DIR, current_input, current_size, "coverage");
        }
        
        // Print progress periodically
        if (i % 100 == 0 || i == iterations - 1) {
            printf("Iteration %d: executions=%d, crashes=%d, timeouts=%d, coverage=%d\n",
                   i, total_executions, crashes, timeouts, covered);
        }
    }
    
    printf("\n=== Fuzzing completed ===\n");
    printf("Total executions: %d\n", total_executions);
    printf("Crashes found: %d\n", crashes);
    printf("Timeouts: %d\n", timeouts);
    printf("New coverage found: %d\n", new_coverage_found);
    printf("Final coverage: %d edges\n", count_covered_edges());
}

// Signal handler for clean shutdown
void handle_signal(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    
    // Free resources
    if (__coverage_map) {
        free(__coverage_map);
        __coverage_map = NULL;
    }
    
    exit(0);
}

// Timeout signal handler
void sigalrm_handler(int sig) {
    (void)sig; // Unused parameter
    if (child_pid > 0) {
        // Set flag and attempt to kill the child
        timed_out = 1;
        // Use killpg for potentially more robust killing if child spawns children
        // killpg(child_pid, SIGKILL);
        kill(child_pid, SIGKILL); // Force kill
    }
}

// Executes the target program in a separate process.
// Returns:
//   0: Normal execution, no crash/timeout
//  -1: Crash detected (signal received)
//  -2: Timeout detected
//  -3: Setup error (fork, shm, file I/O, etc.)
int execute_target(const char* target_path, const uint8_t* data, size_t size) {
    int status = 0;
    int ret_val = -3; // Default to setup error

    // --- 1. Setup Shared Memory ---
    // Create a shared memory segment
    shm_id = shmget(IPC_PRIVATE, COVERAGE_MAP_SIZE, IPC_CREAT | 0600);
    if (shm_id < 0) {
        perror("[-] shmget failed");
        return -3; // Setup error
    }

    // Attach the shared memory segment
    shared_mem_ptr = shmat(shm_id, NULL, 0);
    if (shared_mem_ptr == (void*)-1) {
        perror("[-] shmat failed");
        shmctl(shm_id, IPC_RMID, NULL); // Clean up segment
        return -3; // Setup error
    }

    // Clear the shared memory map (for the child to write into)
    memset(shared_mem_ptr, 0, COVERAGE_MAP_SIZE);

    // --- 2. Setup Input File ---
    char template_name[] = "/tmp/fuzz_input.XXXXXX";
    int input_fd = mkstemp(template_name);
    if (input_fd < 0) {
        perror("[-] mkstemp failed");
        shmdt(shared_mem_ptr);
        shmctl(shm_id, IPC_RMID, NULL);
        return -3; // Setup error
    }

    // Write data to the temporary file
    ssize_t written = write(input_fd, data, size);
    close(input_fd); // Close FD, file still exists
    if (written != (ssize_t)size) {
        fprintf(stderr, "[-] Failed to write complete input to %s\n", template_name);
        unlink(template_name); // Clean up temp file
        shmdt(shared_mem_ptr);
        shmctl(shm_id, IPC_RMID, NULL);
        return -3; // Setup error
    }

    // --- 3. Fork and Execute ---
    timed_out = 0; // Reset timeout flag
    child_pid = fork();

    if (child_pid < 0) {
        perror("[-] fork failed");
        unlink(template_name);
        shmdt(shared_mem_ptr);
        shmctl(shm_id, IPC_RMID, NULL);
        return -3; // Setup error
    }

    if (child_pid == 0) {
        // --- Child Process ---

        // Set process group ID so we can kill the whole group on timeout if needed
        // setpgid(0, 0);

        // Set environment variable for the child to find shared memory
        char shm_env_str[64];
        snprintf(shm_env_str, sizeof(shm_env_str), "%d", shm_id);
        if (setenv("SHM_ID", shm_env_str, 1) != 0) {
             perror("[child] setenv failed");
             exit(1); // Child exits on error
        }

        // Optional: Redirect child's stdout/stderr to /dev/null
        int dev_null_fd = open("/dev/null", O_RDWR);
        if (dev_null_fd >= 0) {
            dup2(dev_null_fd, STDOUT_FILENO);
            dup2(dev_null_fd, STDERR_FILENO);
            close(dev_null_fd);
        }

        // Prepare arguments for execv
        char* argv[] = {(char*)target_path, template_name, NULL};

        // Execute the target program
        execv(target_path, argv);

        // If execv returns, it means an error occurred
        perror("[child] execv failed");
        exit(1); // Child exits on error
    }

    // --- Parent Process ---
    child_pid = child_pid; // Store child PID for signal handler (redundant but clear)

    // Setup timeout alarm
    struct sigaction sa = {0};
    sa.sa_handler = sigalrm_handler;
    sigaction(SIGALRM, &sa, NULL);

    // Calculate timeout in seconds (rounding up)
    int timeout_sec = (TARGET_TIMEOUT_MS + 999) / 1000;
    if (timeout_sec <= 0) timeout_sec = 1; // Minimum 1 second timeout
    alarm(timeout_sec);

    // Wait for the child process to finish or be signaled
    if (waitpid(child_pid, &status, 0) < 0) {
        if (errno == EINTR && timed_out) {
            // Interrupted by our SIGALRM handler
            printf("[!] Timeout detected for PID %d\n", (int)child_pid);
            ret_val = -2; // Timeout
            // Child should have been killed by the handler, but ensure cleanup
        } else {
            perror("[-] waitpid failed");
            // Attempt to kill leftover child if wait failed unexpectedly
            kill(child_pid, SIGKILL);
            ret_val = -3; // Treat as setup/wait error
        }
    } else {
        // Child finished or was signaled *before* timeout triggered alarm
        alarm(0); // Cancel the alarm
        child_pid = -1; // Reset child PID

        if (WIFSIGNALED(status)) {
            // Child terminated by a signal (crash)
            int sig = WTERMSIG(status);
            printf("[-] Crash detected! PID %d terminated by signal %d (%s)\n",
                   (int)child_pid, sig, strsignal(sig)); // child_pid is reset, use status info
            ret_val = -1; // Crash
        } else if (WIFEXITED(status)) {
            // Child exited normally
            int exit_code = WEXITSTATUS(status);
            if (exit_code != 0) {
                printf("[?] Target exited with non-zero status: %d\n", exit_code);
                // Treat non-zero exit as potentially interesting (like timeout/hang)
                // Or could just be normal program behavior. Fuzzer decides how to treat this.
                // For now, treat it like a timeout for saving purposes in the main loop.
                ret_val = exit_code; // Return the positive exit code
            } else {
                 // Normal, clean exit
                 ret_val = 0;
            }
        } else {
             // Should not happen if waitpid succeeded without EINTR
             fprintf(stderr, "[-] Unknown child status: 0x%x\n", status);
             ret_val = -3;
        }
    }

    // --- 4. Retrieve Coverage and Cleanup ---
    if (__coverage_map && shared_mem_ptr != (void*)-1) {
        // Copy coverage from shared memory to the fuzzer's map *after* child is done
        memcpy(__coverage_map, shared_mem_ptr, COVERAGE_MAP_SIZE);
    } else {
        fprintf(stderr, "[-] Warning: Coverage map or shared memory invalid during copy.\n");
    }

    // Detach shared memory
    if (shared_mem_ptr != (void*)-1) {
        shmdt(shared_mem_ptr);
        shared_mem_ptr = (void*)-1;
    }

    // Mark shared memory segment for removal
    // (it will be removed once the last process detaches - which we just did)
    if (shm_id >= 0) {
        shmctl(shm_id, IPC_RMID, NULL);
        shm_id = -1;
    }

    // Delete the temporary input file
    unlink(template_name);

    // Reset global child_pid for safety
    child_pid = -1;
    // Ensure alarm is off
    alarm(0);

    return ret_val;
}


int main(int argc, char** argv) {
    // Parse command line arguments
    if (argc < 2) {
        printf("Usage: %s <target_program> [iterations]\n", argv[0]);
        return 1;
    }
    
    const char* target_path = argv[1];
    int iterations = (argc > 2) ? atoi(argv[2]) : MAX_ITERATIONS;
    
    // Set up signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    // Seed the RNG
    srand(time(NULL));
    
    // Run the fuzzer
    run_fuzzer(target_path, iterations);
    
    // Clean up
    if (__coverage_map) {
        free(__coverage_map);
    }
    
    return 0;
}