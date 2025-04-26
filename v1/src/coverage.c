// filepath: src/coverage.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>

#include "../headers/coverage.h"

// Global shared memory structure
shared_mem_t fuzz_shared_mem = { .shm_id = -1, .map = NULL };
volatile sig_atomic_t child_timed_out = 0;

// Global coverage map accumulates all seen edges
coverage_t global_cov_map[COVERAGE_MAP_SIZE] = {0};

// Alias for the shared coverage map written by the child
#define shared_cov_map fuzz_shared_mem.map

// Initialize shared memory for fuzzing
int setup_shared_memory(void) {
    // Create shared memory segment
    // IPC_PRIVATE ensures a new segment
    // IPC_CREAT | 0600 sets permissions
    fuzz_shared_mem.shm_id = shmget(IPC_PRIVATE, COVERAGE_MAP_SIZE, IPC_CREAT | 0600);
    if (fuzz_shared_mem.shm_id < 0) {
        perror("Fuzzer Error: shmget failed");
        return -1;
    }

    // Attach shared memory segment
    fuzz_shared_mem.map = (coverage_t *)shmat(fuzz_shared_mem.shm_id, NULL, 0);
    if (fuzz_shared_mem.map == (void *)-1) {
        perror("Fuzzer Error: shmat failed");
        // Clean up segment if attach failed
        shmctl(fuzz_shared_mem.shm_id, IPC_RMID, NULL);
        fuzz_shared_mem.shm_id = -1;
        fuzz_shared_mem.map = NULL;
        return -1;
    }

    // Initialize map to zero
    memset(fuzz_shared_mem.map, 0, COVERAGE_MAP_SIZE);
    printf("Fuzzer Info: Shared memory created (ID: %d, Size: %d KB)\n",
           fuzz_shared_mem.shm_id, COVERAGE_MAP_SIZE / 1024);
    return 0;
}

// Detach and remove shared memory
void destroy_shared_memory(void) {
    if (fuzz_shared_mem.map != NULL && fuzz_shared_mem.map != (void *)-1) {
        if (shmdt(fuzz_shared_mem.map) < 0) {
             perror("Fuzzer Warning: shmdt failed");
        }
        fuzz_shared_mem.map = NULL;
    }
    if (fuzz_shared_mem.shm_id >= 0) {
        if (shmctl(fuzz_shared_mem.shm_id, IPC_RMID, NULL) < 0) {
             perror("Fuzzer Warning: shmctl(IPC_RMID) failed");
        }
        fuzz_shared_mem.shm_id = -1;
    }
     printf("Fuzzer Info: Shared memory destroyed.\n");
}

// Reset the coverage map in shared memory (call before each run)
void reset_coverage_map(void) {
    if (fuzz_shared_mem.map) {
        memset(fuzz_shared_mem.map, 0, COVERAGE_MAP_SIZE);
    }
}

// Check if the current map (in shared memory) has new coverage compared to a global map
int has_new_coverage(const coverage_t* global_map) {
    if (!fuzz_shared_mem.map || !global_map) {
        return 0; // Cannot compare if maps are invalid
    }

    for (int i = 0; i < COVERAGE_MAP_SIZE; i++) {
        // Check if a bit is set in the shared map but not in the global map
        if (fuzz_shared_mem.map[i] > 0 && global_map[i] == 0) {
            return 1; // Found new coverage
        }
    }
    return 0; // No new coverage found
}

// Update a global map with coverage found in the shared memory map
// Simple version: just mark presence (1) if hit count > 0
void update_global_coverage(coverage_t* global_map) {
     if (!fuzz_shared_mem.map || !global_map) {
        return; // Cannot update if maps are invalid
    }

    for (int i = 0; i < COVERAGE_MAP_SIZE; i++) {
        if (fuzz_shared_mem.map[i] > 0) {
            global_map[i] = 1; // Mark as covered in the global map
            // Could also use: global_map[i] |= fuzz_shared_mem.map[i]; for hit counts
        }
    }
}

// Calculate a fitness score based on the coverage map (usually the one in shared memory)
// Note: Pass global_map to check for novelty bonus
double calculate_coverage_fitness(const coverage_t* current_map, const coverage_t* global_map) {
    if (!current_map || !global_map) return 0.0;

    int covered_edges = 0;
    int new_edges = 0;

    for (int i = 0; i < COVERAGE_MAP_SIZE; i++) {
        if (current_map[i] > 0) {
            covered_edges++;
            // Check if this edge is new compared to the global map *before* update
            if (global_map[i] == 0) {
                new_edges++;
            }
        }
    }

    // Base fitness on coverage count
    double fitness = (double)covered_edges;

    // Heavily reward new edge discovery
    if (new_edges > 0) {
        fitness += new_edges * 10.0; // Bonus factor for new edges
    }

    // Optional: Consider hit counts for density? (e.g., AFL uses bucketed counts)

    return fitness;
}

// Count the number of edges covered in a map
int count_covered_edges(const coverage_t* map) {
     if (!map) return 0;

    int covered = 0;
    for (int i = 0; i < COVERAGE_MAP_SIZE; i++) {
        if (map[i] > 0) {
            covered++;
        }
    }
    return covered;
}

// Dump coverage summary for a given map
void dump_coverage_summary(const coverage_t* map) {
    if (!map) {
        printf("Coverage map not available\n");
        return;
    }

    int covered = count_covered_edges(map);
    // Calculate density (percentage)
    double density = (COVERAGE_MAP_SIZE > 0) ? (double)covered * 100.0 / COVERAGE_MAP_SIZE : 0.0;

    printf("Coverage summary: %d of %d potential edges covered (%.2f%% density)\n",
           covered, COVERAGE_MAP_SIZE, density);
}

// Evaluate coverage: count new edges in shared_cov_map, merge into global_cov_map, return new edge count
int evaluate_coverage(void) {
    if (!shared_cov_map) return 0;
    int new_edges = 0;
    for (int i = 0; i < COVERAGE_MAP_SIZE; i++) {
        if (shared_cov_map[i] > 0 && global_cov_map[i] == 0) {
            new_edges++;
            global_cov_map[i] = 1;
        }
    }
    return new_edges;
}

// Merge a run's coverage map into the global map without counting new edges
void merge_global_coverage(const coverage_t* run_cov) {
    if (!run_cov) return;
    for (int i = 0; i < COVERAGE_MAP_SIZE; i++) {
        if (run_cov[i] > 0) {
            global_cov_map[i] = 1;
        }
    }
}

// Stub for saving coverage on crashes/timeouts
void __coverage_save(void) {
    if (fuzz_shared_mem.map) {
        // Merge current run's coverage into global map
        for (int i = 0; i < COVERAGE_MAP_SIZE; i++) {
            if (fuzz_shared_mem.map[i] > 0) global_cov_map[i] = 1;
        }
    }
}

// Generic signal handler
void fuzzer_signal_handler(int sig) {
    if (sig == SIGALRM) {
        // Set flag indicating timeout
        child_timed_out = 1;
        // The waitpid in executeTargetInt needs to check this flag
    }
    // Can add handlers for SIGINT/SIGTERM for graceful shutdown
    // if (sig == SIGINT || sig == SIGTERM) {
    //     destroy_shared_memory();
    //     // Any other cleanup
    //     exit(0);
    // }
}