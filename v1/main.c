// filepath: src/main.c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <libgen.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "headers/fuzz.h"
#include "headers/io.h"
#include "headers/testcase.h"
#include "headers/target.h"
#include "headers/generational.h"
#include "headers/corpus.h"
#include "headers/coverage.h"

#define MAX_ITERATIONS 10000
#define CORPUS_DIR "corpus"
#define CRASH_DIR "crashes"    // Directory for crashing inputs
#define TIMEOUT_DIR "timeouts" // Directory for timeout inputs
#define PROGRESS_FILE "fuzzing_progress.csv"
#define TARGET_TIMEOUT_MS 1000
#define FUZZER_EXEC_ERROR -999

int minRange = INT_MIN;
int maxRange = INT_MAX;
coverage_t *global_coverage_map = NULL;
const char *target_exe_path_global = NULL; // Store path for signal handler
int random_mode = 0;
int genetic_mode = 0;

// Function to save unique findings (crashes/timeouts)
void save_finding(int input_val, const char *finding_type)
{
    char finding_dir[PATH_MAX];
    char filename[PATH_MAX];
    struct stat st = {0};

    snprintf(finding_dir, sizeof(finding_dir), "%s", finding_type); // e.g., "crashes"

    // Create directory if it doesn't exist
    if (stat(finding_dir, &st) == -1)
    {
        if (mkdir(finding_dir, 0755) == -1 && errno != EEXIST)
        {
            fprintf(stderr, "Warning: Failed to create %s directory: %s\n", finding_dir, strerror(errno));
            return; // Cannot save
        }
    }

    // Use input value for filename (careful with large/negative values)
    // Consider hashing input if values are problematic for filenames
    snprintf(filename, sizeof(filename), "%s/finding_%d_%ld", finding_dir, input_val, (long)time(NULL));

    // Check if file already exists (simple check, might collide)
    if (stat(filename, &st) == 0)
    {
        // printf("Note: Finding file %s already exists, not overwriting.\n", filename);
        return; // Don't overwrite for now
    }

    FILE *fp = fopen(filename, "w");
    if (fp)
    {
        fprintf(fp, "%d\n", input_val);
        fclose(fp);
        printf(">>> Saved %s input to: %s <<<\n", finding_type, filename);
    }
    else
    {
        fprintf(stderr, "Warning: Failed to save finding to %s: %s\n", filename, strerror(errno));
    }
}

void graceful_shutdown(int sig)
{
    fprintf(stderr, "[Main] Signal %d received, shutting down...\n", sig); // Use stderr
    destroy_shared_memory();
    if (global_coverage_map)
        free(global_coverage_map);
    if (target_exe_path_global)
        cleanup_target(target_exe_path_global);
    exit(0);
}

// Function to perform random fuzzing
void randomFuzzing(const char *target_exe, int iterations, int min_r, int max_r)
{
    fprintf(stderr, "[Main] Starting randomFuzzing...\n");
    FILE *progress_file = fopen(PROGRESS_FILE, "a"); // Append mode
    if (progress_file && ftell(progress_file) == 0)
    { // Write header only if file is new/empty
        fprintf(progress_file, "Iteration,Coverage,Mode,CorpusSize,Crashes,Timeouts\n");
    }

    global_coverage_map = calloc(COVERAGE_MAP_SIZE, sizeof(coverage_t));
    if (!global_coverage_map)
    { /* error */
        if (progress_file)
            fclose(progress_file);
        return;
    }

    int crashes = 0;
    int timeouts = 0;

    for (int i = 0; i < iterations; i++)
    {
        long long range_size = (long long)max_r - min_r + 1;
        int random_input = min_r;
        if (range_size > 0)
        { // Avoid modulo by zero or negative
            random_input = min_r + (rand() % range_size);
        }

        int status = execute_target_fork(target_exe, random_input, TARGET_TIMEOUT_MS);

        update_global_coverage(global_coverage_map);

        // **FIX:** Check status codes correctly
        if (status < 0 && status != -SIGALRM && status != FUZZER_EXEC_ERROR)
        { // Negative other than timeout/internal error = Signal Crash
            crashes++;
            printf("!!! Random Crash found with input: %d (Iteration: %d, Signal: %d) !!!\n", random_input, i, -status);
            save_finding(random_input, CRASH_DIR);
        }
        else if (status == -SIGALRM)
        {
            timeouts++;
            printf("!!! Random Timeout found with input: %d (Iteration: %d) !!!\n", random_input, i);
            save_finding(random_input, TIMEOUT_DIR);
        }
        else if (status > 0)
        {
            // Optional: Log non-zero exits?
            // printf("Info: Random run exited with code %d (Input: %d)\n", status, random_input);
        }
        else if (status == FUZZER_EXEC_ERROR)
        {
            fprintf(stderr, "Warning: Fuzzer execution error for input %d\n", random_input);
        }

        if (i % 100 == 0 || i == iterations - 1)
        {
            int current_total_coverage = count_covered_edges(global_coverage_map);
            printf("Iter %d: Total Cov %d, Crashes %d, Timeouts %d\n",
                   i, current_total_coverage, crashes, timeouts);
            if (progress_file)
            {
                // Use dummy values for corpus size in random mode
                fprintf(progress_file, "%d,%d,random,0,%d,%d\n", i, current_total_coverage, crashes, timeouts);
                fflush(progress_file);
            }
        }
    }
    printf("\n=== Random fuzzing completed ===\n");
    printf("Total iterations: %d\n", iterations);
    printf("Final total coverage: %d paths\n", count_covered_edges(global_coverage_map));
    printf("Crashes: %d, Timeouts: %d\n", crashes, timeouts);
    dump_coverage_summary(global_coverage_map);

    if (progress_file)
        fclose(progress_file);
}

// Function to perform grey box fuzzing
void greyBoxFuzzing(const char *target_exe, int iterations, int min_r, int max_r)
{
    fprintf(stderr, "[Main] Starting greyBoxFuzzing...\n");
    FILE *progress_file = fopen(PROGRESS_FILE, "a"); // Append mode
    if (progress_file && ftell(progress_file) == 0)
    { // Write header only if file is new/empty
        fprintf(progress_file, "Iteration,Coverage,Mode,CorpusSize,Crashes,Timeouts\n");
    }

    // Use static global_cov_map in coverage.c to track coverage
    fprintf(stderr, "[Main] Using static global_cov_map for coverage tracking...\n");

    fprintf(stderr, "[Main] Initializing corpus...\n");
    if (initializeCorpus(CORPUS_DIR) != 0)
    {
        fprintf(stderr, "[Main] Error: Failed to initialize corpus\n");
        if (progress_file)
            fclose(progress_file);
        return;
    }

    fprintf(stderr, "[Main] Initializing populations...\n");
    initializePopulations();

    printf("Starting grey-box fuzzing with coverage feedback...\n");

    // Initialize population & evaluate initial inputs
    fprintf(stderr, "[Main] Initializing population & evaluating initial inputs...\n");
    int initial_crashes = 0;
    int initial_timeouts = 0;
    for (int i = 0; i < POPULATION_SIZE; i++) {
        long long range_size = (long long)max_r - min_r + 1;
        population[i].input_value = min_r;
        if (range_size > 0)
        {
            population[i].input_value = min_r + (rand() % range_size);
        }

        int status = execute_target_fork(target_exe, population[i].input_value, TARGET_TIMEOUT_MS);

        // Evaluate coverage for this input
        int new_edges = evaluate_coverage();
        population[i].fitness_score = new_edges;
        // Snapshot coverage map
        if (fuzz_shared_mem.map) memcpy(population[i].coverage_map, fuzz_shared_mem.map, COVERAGE_MAP_SIZE);
        else memset(population[i].coverage_map, 0, COVERAGE_MAP_SIZE);

        // If this input discovered new edges, add to corpus
        if (new_edges > 0) {
            saveToCorpus(population[i].input_value, fuzz_shared_mem.map, population[i].fitness_score, 1);
        }

        // Count initial crashes/timeouts from population seeding
        if (status < 0 && status != -SIGALRM && status != FUZZER_EXEC_ERROR)
        {
            initial_crashes++;
            save_finding(population[i].input_value, CRASH_DIR);
        }
        else if (status == -SIGALRM)
        {
            initial_timeouts++;
            save_finding(population[i].input_value, TIMEOUT_DIR);
        }
    }

    fprintf(stderr, "[Main] Population initialized. Initial corpus size: %d\n", getCorpusSize());

    if (progress_file)
    {
        int initial_coverage = count_covered_edges(global_coverage_map);
        fprintf(progress_file, "0,%d,greybox,%d,%d,%d\n", initial_coverage, getCorpusSize(), initial_crashes, initial_timeouts);
        fflush(progress_file);
    }

    int last_corpus_update = 0;
    int crashes = initial_crashes; // Start counting from initial phase
    int timeouts = initial_timeouts;

    fprintf(stderr, "[Main] Starting main fuzzing loop...\n");
    for (int iter = 1; iter <= iterations; iter++)
    { // Start iter from 1
        // Status update per iteration
        printf("[Main] Iteration %d/%d - Coverage: %d, Corpus: %d\n",
               iter, iterations,
               count_covered_edges(global_cov_map),
               getCorpusSize());

        int input_val;
        int generated_new = 0; // Flag if *corpus* got a new entry this iteration

        // --- Input Selection Strategy ---
        // Simplified: 50% Corpus (mutate/crossover), 50% GA
        if (getCorpusSize() > 0 && rand() % 2 == 0)
        {
            CorpusEntry *entry = selectCorpusEntry();
            if (!entry) {
                fprintf(stderr, "[Main %d] Error: selectCorpusEntry returned NULL despite corpus size > 0. Skipping corpus step.\n", iter);
                // Force GA path or skip iteration? Let's try forcing GA.
                goto use_ga; // Jump to the 'else' block for GA
                // continue; // Alternative: Skip this iteration
           }

            int mutation_type = rand() % 10;
            if (mutation_type < 7 || getCorpusSize() < 2)
            { // Mutate/Havoc
                input_val = mutateHavoc(entry->input_value);
            }
            else
            { // Crossover
                CorpusEntry *entry2 = selectCorpusEntry();
                if (!entry2 || entry == entry2)
                    input_val = mutateHavoc(entry->input_value); // Avoid self-crossover
                else
                    input_val = crossover(entry->input_value, entry2->input_value);
            }
        }
        else
        { // GA-based generation
use_ga:
            generateNewPopulation(population, POPULATION_SIZE, next_generation, min_r, max_r);
            for (int i = 0; i < POPULATION_SIZE; i++) {
                int status_ga = execute_target_fork(target_exe, next_generation[i].input_value, TARGET_TIMEOUT_MS);
                // Evaluate coverage and fitness
                int new_edges_ga = evaluate_coverage();
                next_generation[i].fitness_score = new_edges_ga;
                // Snapshot coverage
                if (fuzz_shared_mem.map) memcpy(next_generation[i].coverage_map, fuzz_shared_mem.map, COVERAGE_MAP_SIZE);
                else memset(next_generation[i].coverage_map, 0, COVERAGE_MAP_SIZE);
                // If new edges discovered, record and add to corpus
                if (new_edges_ga > 0) {
                    saveToCorpus(next_generation[i].input_value, fuzz_shared_mem.map, new_edges_ga, 1);
                    generated_new = 1;
                    last_corpus_update = iter;
                }
                // **FIX:** Check status codes correctly for GA runs
                if (status_ga < 0 && status_ga != -SIGALRM && status_ga != FUZZER_EXEC_ERROR)
                {
                    crashes++;
                    save_finding(next_generation[i].input_value, CRASH_DIR);
                }
                else if (status_ga == -SIGALRM)
                {
                    timeouts++;
                    save_finding(next_generation[i].input_value, TIMEOUT_DIR);
                }
                // ...other status handling...
            }
            // Replace population
            for (int i = 0; i < POPULATION_SIZE; ++i)
            {
                population[i].input_value = next_generation[i].input_value;
                population[i].fitness_score = next_generation[i].fitness_score;
                memcpy(population[i].coverage_map, next_generation[i].coverage_map, COVERAGE_MAP_SIZE);
            }
            input_val = population[rand() % POPULATION_SIZE].input_value; // Select one from new pop for main check
        }

        // Clamp input value (maybe redundant if mutation/crossover handle it)
        if (input_val < min_r)
            input_val = min_r;
        if (input_val > max_r)
            input_val = max_r;

        // --- Execute the chosen input ---
        int status = execute_target_fork(target_exe, input_val, TARGET_TIMEOUT_MS);

        // --- Check results for new coverage ---
        if (status != FUZZER_EXEC_ERROR && fuzz_shared_mem.map) {
            int new_edges = evaluate_coverage();
            if (new_edges > 0) {
                printf("+++ New coverage: %d new edges with input %d (Iteration: %d) +++\n", new_edges, input_val, iter);
                saveToCorpus(input_val, fuzz_shared_mem.map, new_edges, 1);
                generated_new = 1;
                last_corpus_update = iter;
            }
        }

        // --- Handle crashes/timeouts ---

        if (status < 0 && status != -SIGALRM && status != FUZZER_EXEC_ERROR)
        { // Crash
            crashes++;
            printf("!!! Crash found with input: %d (Iteration: %d, Signal: %d) !!!\n", input_val, iter, -status);
            save_finding(input_val, CRASH_DIR);
            // Also add crashing input to corpus? Might be useful for mutation.
            if (fuzz_shared_mem.map)
            {                                                         // Save with coverage map if available
                saveToCorpus(input_val, fuzz_shared_mem.map, 0.0, 1); // Low fitness, but interesting
            }
            else
            {
                saveToCorpus(input_val, NULL, 0.0, 1);
            }
            generated_new = 1; // Counts as corpus update for stagnation check
        }
        else if (status == -SIGALRM)
        { // Timeout
            timeouts++;
            printf("!!! Timeout found with input: %d (Iteration: %d) !!!\n", input_val, iter);
            save_finding(input_val, TIMEOUT_DIR);
            if (fuzz_shared_mem.map)
                saveToCorpus(input_val, fuzz_shared_mem.map, 0.0, 1);
            else
                saveToCorpus(input_val, NULL, 0.0, 1);
            generated_new = 1;
        }
        else if (status > 0)
        {   // Non-zero exit
            // printf("Info: Input %d exited with code %d\n", input_val, status);
        }
        else if (status == FUZZER_EXEC_ERROR)
        { // Fuzzer internal error
            fprintf(stderr, "Warning: Fuzzer execution error for input %d\n", input_val);
        }

        // --- Periodic Actions ---
        if (iter % 100 == 0 || iter == iterations || generated_new)
        {
            int current_total_coverage = count_covered_edges(global_cov_map);
            int corpus_s = getCorpusSize();
            printf("Iter %d: Total Cov %d, Corpus %d, Crashes %d, Timeouts %d\n",
                   iter, current_total_coverage, corpus_s, crashes, timeouts);
            if (progress_file)
            {
                fprintf(progress_file, "%d,%d,greybox,%d,%d,%d\n",
                        iter, current_total_coverage, corpus_s, crashes, timeouts);
                fflush(progress_file);
            }
        }

        // Corpus minimization (optional, based on stagnation)
        if (getCorpusSize() > 50 && iter - last_corpus_update > 2000)
        {
            printf("Minimizing corpus (stagnant)...\n");
            minimizeCorpus();
            last_corpus_update = iter;
        }
    }
    fprintf(stderr, "[Main] Fuzzing loop finished.\n"); // Log loop exit
    printf("\n=== Grey box fuzzing completed ===\n");
    printf("Total iterations: %d\n", iterations);
    printf("Crashes: %d, Timeouts: %d\n", crashes, timeouts);
    printCorpusStats();
    dump_coverage_summary(global_coverage_map);

    cleanupPopulations();
    cleanupCorpus();
    if (progress_file)
        fclose(progress_file);
}

// Main function
int main(int argc, char *argv[])
{
    fprintf(stderr, "[Main] Fuzzer starting...\n");
    int opt;
    const char *filename = NULL;

    fprintf(stderr, "[Main] Parsing arguments...\n");
    while ((opt = getopt(argc, argv, "rgi:o:n:x:")) != -1)
    {
        switch (opt)
        {
        case 'r':
            random_mode = 1;
            fprintf(stderr, "[Main] Arg: Random mode enabled\n"); // Add log
            break;
        case 'g':
            genetic_mode = 1;
            fprintf(stderr, "[Main] Arg: Genetic mode enabled\n");
            break;
        case 'i':                                                               // Input target source file
            filename = optarg;                                                  // optarg contains the argument to -i
            fprintf(stderr, "[Main] Arg: Target file set to '%s'\n", filename); // Add log
            break;
        // Add options for min/max range?
        case 'n':
            // minRange = atoi(optarg); // Example if using -n
            fprintf(stderr, "[Main] Arg: Option -n found with '%s'\n", optarg); // Add log
            break;
        case 'x':
            // maxRange = atoi(optarg); // Example if using -x
            fprintf(stderr, "[Main] Arg: Option -x found with '%s'\n", optarg); // Add log
            break;
        case 'o':                                                               // Output directory? (unused for now)
            fprintf(stderr, "[Main] Arg: Option -o found with '%s'\n", optarg); // Add log
            break;
        case '?': // Handle unknown options or missing arguments
            fprintf(stderr, "[Main] Warning: Unknown option or missing argument for '-%c'\n", optopt);
            // You might want to exit here depending on desired behavior
            break;
        default:
            // Should not happen with getopt
            fprintf(stderr, "[Main] Error: Unexpected argument parsing result.\n");
            return 1;
        }
    } // Keep options parsing

    if (random_mode && genetic_mode) {
        fprintf(stderr, "[Main] Warning: Both random and genetic modes enabled, defaulting to genetic mode\n");
        random_mode = 0;
    }

    if (!filename)
    {
        fprintf(stderr, "[Main] Error: Target source file (-i) is required.\n");
        return 1;
    }
    fprintf(stderr, "[Main] Target file: %s\n", filename);

    fprintf(stderr, "[Main] Resolving paths...\n");
    char *fullPath = realpath(filename, NULL);
    if (!fullPath)
    { /* error */
        return 1;
    }
    char *temp_path_dir = strdup(fullPath);
    char *temp_path_base = strdup(fullPath);
    if (!temp_path_dir || !temp_path_base)
    { /* error */ /* free */
        return 1;
    }
    const char *source_dir = dirname(temp_path_dir);
    const char *base_filename = basename(temp_path_base);

    char target_exe_name[PATH_MAX], base_name_no_ext[PATH_MAX], target_exe_path[PATH_MAX];
    // ... (generate target_exe_path as before) ...
    snprintf(target_exe_name, sizeof(target_exe_name), "%s_fuzz", base_name_no_ext); // Simplified generation
    snprintf(target_exe_path, sizeof(target_exe_path), "%s/%s", source_dir, target_exe_name);
    fprintf(stderr, "[Main] Target source dir: %s\n", source_dir);
    fprintf(stderr, "[Main] Target base name: %s\n", base_filename);
    fprintf(stderr, "[Main] Output executable: %s\n", target_exe_path);
    target_exe_path_global = target_exe_path; // Store globally for cleanup handler

    printf("Fuzzer Info:\n"); /* ... print info ... */

    fprintf(stderr, "[Main] Compiling target...\n");
    if (compile_target_with_clang_coverage(source_dir, base_filename, target_exe_name) != 0)
    {
        fprintf(stderr, "[Main] Error: Failed to compile target.\n");
        free(fullPath);
        free(temp_path_dir);
        free(temp_path_base);
        return 1;
    }
    fprintf(stderr, "[Main] Target compiled successfully.\n");

    fprintf(stderr, "[Main] Seeding RNG...\n");
    unsigned int seed = time(NULL) ^ getpid();
    fprintf(stderr, "[Main] Using seed: %u\n", seed);
    srand(seed);

    fprintf(stderr, "[Main] Setting up shared memory...\n");
    if (setup_shared_memory() != 0)
    {
        fprintf(stderr, "[Main] Error: Failed to set up shared memory.\n"); // Use stderr
        free(fullPath);
        free(temp_path_dir);
        free(temp_path_base);
        cleanup_target(target_exe_path); // Clean up compiled target on error
        return 1;
    }
    fprintf(stderr, "[Main] Shared memory setup complete.\n");

    fprintf(stderr, "[Main] Setting up signal handlers...\n");
    signal(SIGINT, graceful_shutdown);
    signal(SIGTERM, graceful_shutdown);

    if (random_mode)
    {
        fprintf(stderr, "[Main] Starting fuzzing mode: Random\n");
        randomFuzzing(target_exe_path, MAX_ITERATIONS, minRange, maxRange);
    }
    else /* Genetic mode by default */
    {
        fprintf(stderr, "[Main] Starting fuzzing mode: Genetic\n");
        greyBoxFuzzing(target_exe_path, MAX_ITERATIONS, minRange, maxRange);
    }

    fprintf(stderr, "[Main] Fuzzing finished. Cleaning up...\n");
    destroy_shared_memory();
    cleanup_target(target_exe_path);
    if (global_coverage_map)
        free(global_coverage_map);
    free(fullPath);
    free(temp_path_dir);
    free(temp_path_base);
    fprintf(stderr, "[Main] Cleanup complete. Exiting.\n");
    return 0;
}