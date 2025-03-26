// filepath: headers/coverage.h
#ifndef COVERAGE_H
#define COVERAGE_H

#include <stdint.h> // For uint8_t
#include <signal.h> // For sig_atomic_t

// Define the size of the coverage map (must match coverage_runtime.c)
// Needs to be a power of 2 for the modulo arithmetic. 64KB is standard.
#define COVERAGE_MAP_SIZE (1 << 16)

// Type alias for coverage map entries (usually bytes)
typedef uint8_t coverage_t;

// --- Fuzzer-Side Shared Memory Management ---

// Structure to hold shared memory info
typedef struct {
    int shm_id;         // Shared memory ID
    coverage_t *map;    // Pointer to the shared memory map
    // Add other shared data if needed (e.g., crash status)
} shared_mem_t;

// Global shared memory structure (or pass it around)
extern shared_mem_t fuzz_shared_mem;
extern volatile sig_atomic_t child_timed_out; // Flag for timeouts

// Initialize shared memory for fuzzing
int setup_shared_memory(void);

// Detach and remove shared memory
void destroy_shared_memory(void);

// Reset the coverage map in shared memory (call before each run)
void reset_coverage_map(void);

// --- Fuzzer-Side Coverage Analysis ---

// Check if the current map (in shared memory) has new coverage compared to a global map
int has_new_coverage(const coverage_t* global_map);

// Update a global map with coverage found in the shared memory map
void update_global_coverage(coverage_t* global_map);

// Calculate a fitness score based on the coverage map (usually the one in shared memory)
// Note: Pass global_map to check for novelty bonus
double calculate_coverage_fitness(const coverage_t* current_map, const coverage_t* global_map);

// Count the number of edges covered in a map
int count_covered_edges(const coverage_t* map);

// Dump coverage summary
void dump_coverage_summary(const coverage_t* map);


// --- Signal Handling ---
// Generic signal handler (can be used for timeouts, etc.)
void fuzzer_signal_handler(int sig);

#endif // COVERAGE_H