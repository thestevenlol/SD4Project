// filepath: src/coverage_runtime.c
#define _GNU_SOURCE // For pipe2
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

// Define the shared memory size (must match fuzzer)
#define COVERAGE_MAP_SIZE (1 << 16) // 64KB

// Shared memory pointer - global within this runtime
static uint8_t *__coverage_map_ptr = NULL;
static int __shm_id = -1;

// AFL-style edge coverage requires tracking the previous location.
// Use thread-local storage for multi-threaded targets.
static __thread uint32_t __prev_loc = 0;

// Pipe file descriptor for target -> fuzzer communication (e.g., ready signal)
#define FUZZ_FD 198 // Arbitrary but potentially conflicting FD, use higher if needed
// This FD is inherited from the fuzzer via fork

// Called once at program startup.
// 'start' and 'stop' delimit the guard locations.
void __sanitizer_cov_trace_pc_guard_init(uint32_t *start, uint32_t *stop) {
    // Don't instrument the instrumentation itself
    if (start == stop || *start) return;

    // 1. Get Shared Memory ID from environment variable set by fuzzer
    const char *shm_id_str = getenv("__AFL_SHM_ID"); // Use AFL's standard env var name
    if (!shm_id_str) {
        // Fallback or error if not running under the fuzzer?
        // For now, let's assume it will always be set.
        // fprintf(stderr, "Target Error: __AFL_SHM_ID not set!\n");
        // If we allow standalone execution, maybe alloc a dummy map?
        // For now, just exit or proceed without coverage if not set.
        return; // No coverage if not fuzzed
    }

    __shm_id = atoi(shm_id_str);
    if (__shm_id < 0) {
        // fprintf(stderr, "Target Error: Invalid __AFL_SHM_ID value: %s\n", shm_id_str);
        return; // Invalid ID
    }

    // 2. Attach to the shared memory segment
    __coverage_map_ptr = (uint8_t *)shmat(__shm_id, NULL, 0);
    if (__coverage_map_ptr == (void *)-1) {
        // perror("Target Error: shmat failed");
        __coverage_map_ptr = NULL; // Ensure it's NULL on failure
        return; // Cannot proceed without shared memory
    }

    // 3. Instrument all guard points to call __sanitizer_cov_trace_pc_guard
    // The guards initially point to this function (__sanitizer_cov_trace_pc_guard).
    // We transform them into indices.
    uint32_t N = 0;
    for (uint32_t *x = start; x < stop; x++) {
         *x = ++N; // Assign unique index N to *x (starting from 1)
    }

    // Optional: Signal fuzzer that initialization is complete via pipe
    // char fuzz_ready_buf[4] = {'F','U','Z','Z'};
    // write(FUZZ_FD, fuzz_ready_buf, 4); // Fuzzer should read this

    // fprintf(stderr, "Target Info: Attached SHM ID %d, Map Ptr: %p, Guards: %u\n",
            // __shm_id, __coverage_map_ptr, N); // Debug
}

// Called on every edge execution (if trace-pc-guard is used).
// 'guard' now holds the unique index we assigned in the init function.
void __sanitizer_cov_trace_pc_guard(uint32_t *guard) {
    // If shared memory setup failed, do nothing.
    if (!__coverage_map_ptr) return;

    // Map the edge transition to a coverage map index (AFL algorithm)
    // Use guard value directly as 'current_loc'. Note guard points to the index value.
    uint32_t current_loc = *guard;

    // Simplified AFL hash: current_location ^ previous_location
    // Ensure the index stays within the map bounds.
    uint32_t map_idx = (current_loc ^ __prev_loc) % COVERAGE_MAP_SIZE;

    // Increment the hit count for this edge transition.
    // Use saturating increment (stops at 255).
    if (__coverage_map_ptr[map_idx] < 255) {
         __coverage_map_ptr[map_idx]++;
    }

    // Update previous location, shifted right to keep the hash spread out.
    // Shift amount can be tuned, 1 is common.
    __prev_loc = current_loc >> 1;
}

// Called on every comparison if trace-cmp instrumentation is enabled.
// Records (Arg1 ^ Arg2) into the coverage map.
void __sanitizer_cov_trace_cmp(uint64_t Arg1, uint64_t Arg2) {
    if (!__coverage_map_ptr) return;
    uint32_t idx = (uint32_t)((Arg1 ^ Arg2) % COVERAGE_MAP_SIZE);
    if (__coverage_map_ptr[idx] < 255) {
        __coverage_map_ptr[idx]++;
    }
}

// Define __VERIFIER_nondet_int if the target needs it.
// **IMPORTANT**: This makes the target non-deterministic based on fuzz input.
// The fuzzer needs to provide this input via stdin/file now.
// We read it once per execution.
static int fuzz_input_value = 0;
static int fuzz_input_read = 0;

int __VERIFIER_nondet_int() {
    if (!fuzz_input_read) {
        // Read the integer input provided by the fuzzer (e.g., from stdin)
        // This assumes the fuzzer writes the integer as text to stdin.
        if (scanf("%d", &fuzz_input_value) != 1) {
             // Handle error: Use a default or report?
             // fprintf(stderr, "Target Warning: Failed to read fuzzer input from stdin\n");
             fuzz_input_value = 0; // Default value on read error
        }
        fuzz_input_read = 1;
    }
    return fuzz_input_value;
}