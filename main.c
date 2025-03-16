#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <libgen.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "headers/fuzz.h"
#include "headers/lex.h"
#include "headers/io.h"
#include "headers/testcase.h"
#include "headers/target.h"
#include "headers/range.h"
#include "headers/generational.h"
#include "headers/corpus.h"
#include "headers/coverage.h"

#define MAX_ITERATIONS 10000
#define CORPUS_DIR "corpus"
#define PROGRESS_FILE "fuzzing_progress.csv"

// Function to perform random fuzzing without coverage guidance
void randomFuzzing(int iterations, int min_range, int max_range, char* filename) {
    printf("\n=== Starting random fuzzing (no coverage guidance) ===\n");
    
    FILE *progress_file = fopen(PROGRESS_FILE, "w");
    if (progress_file) {
        fprintf(progress_file, "Iteration,Coverage,Mode\n");
    }
    
    // Initialize baseline coverage map
    coverage_t* baseline_coverage = calloc(COVERAGE_MAP_SIZE, sizeof(coverage_t));
    
    // Reset coverage maps at start
    __coverage_reset();
    resetGlobalCoverageMap();
    
    for (int i = 0; i < iterations; i++) {
        // Generate a completely random input within range
        int random_input = min_range + rand() % (max_range - min_range + 1);
        
        // Reset coverage before each test
        __coverage_reset();
        
        // Execute with the random input
        executeTargetInt(random_input, filename);
        
        // Track if we found new coverage
        if (hasNewCoverage(__coverage_map, baseline_coverage)) {
            updateGlobalCoverage(baseline_coverage);
            printf("Found new coverage with input: %d (Iteration: %d)\n", random_input, i);
        }
        
        // Save progress data every 100 iterations
        if (progress_file && (i % 100 == 0 || i == iterations - 1)) {
            int covered = __coverage_count();
            fprintf(progress_file, "%d,%d,random\n", i, covered);
            printf("Iteration %d: Coverage %d paths\n", i, covered);
        }
    }
    
    // Print final statistics
    int total_coverage = 0;
    for (int i = 0; i < COVERAGE_MAP_SIZE; i++) {
        if (baseline_coverage[i] > 0) {
            total_coverage++;
        }
    }
    
    printf("\n=== Random fuzzing completed ===\n");
    printf("Total iterations: %d\n", iterations);
    printf("Final coverage: %d paths\n", total_coverage);
    
    free(baseline_coverage);
    
    if (progress_file) {
        fclose(progress_file);
    }
}

// Function to perform grey box fuzzing with coverage guidance
void greyBoxFuzzing(int iterations, int min_range, int max_range, char* filename) {
    FILE *progress_file = fopen(PROGRESS_FILE, "w");
    if (progress_file) {
        fprintf(progress_file, "Iteration,Coverage,Mode\n");
    }
    
    // Initialize coverage and corpus tracking
    __coverage_init();
    __coverage_reset(); // Reset coverage at start
    initializePopulations();
    resetGlobalCoverageMap(); // Reset global coverage map at start
    initializeCorpus(CORPUS_DIR);
    
    printf("Starting grey-box fuzzing with coverage feedback...\n");
    
    // 1. Initialize population with random inputs
    printf("Initializing population...\n");
    for (int i = 0; i < POPULATION_SIZE; i++)
    {
        __coverage_reset(); // Reset before each individual evaluation
        
        population[i].input_value = min_range + rand() % (max_range - min_range + 1);
        
        // Reset coverage map for this run
        resetCoverageMap(population[i].coverage_map);
        
        // Execute target and collect coverage
        executeTargetInt(population[i].input_value, filename);
        
        // Copy coverage data from __coverage_map to individual's coverage map
        if (__coverage_map) {
            memcpy(population[i].coverage_map, __coverage_map, COVERAGE_MAP_SIZE * sizeof(coverage_t));
        }
        
        // Calculate fitness based on coverage
        population[i].fitness_score = calculateCoverageFitness(population[i].coverage_map);
        
        // Check if input discovered new coverage
        if (hasNewCoverage(population[i].coverage_map, global_coverage_map)) {
            updateGlobalCoverage(population[i].coverage_map);
            
            // Save interesting inputs to corpus
            saveToCorpus(population[i].input_value,
                         population[i].coverage_map,
                         population[i].fitness_score,
                         1); // Mark as interesting
        }
    }

    // Reset coverage before main fuzzing loop
    __coverage_reset();
    
    // Save initial progress data
    if (progress_file) {
        int covered = __coverage_count();
        fprintf(progress_file, "0,%d,greybox\n", covered);
    }
    
    // Main fuzzing loop
    int last_corpus_update = 0;
    
    printf("\n=== Starting main fuzzing loop ===\n");
    
    for (int iter = 0; iter < iterations; iter++) {
        // Reset coverage at start of each iteration
        __coverage_reset();
        
        if (iter % 100 == 0) {
            printf("Iteration %d, corpus size: %d\n", iter, getCorpusSize());
            
            // Save progress data
            if (progress_file) {
                int covered = __coverage_count();
                fprintf(progress_file, "%d,%d,greybox\n", iter, covered);
            }
        }
        
        // Periodically minimize corpus after enough new inputs
        if (getCorpusSize() > 0 && iter - last_corpus_update > 500) {
            printf("Minimizing corpus...\n");
            minimizeCorpus();
            last_corpus_update = iter;
        }
        
        // 2. Generational loop with crossover and mutation
        for (int generation = 0; generation < NUM_GENERATIONS; generation++)
        {
            // Reset coverage before each generation
            __coverage_reset();
            
            // Generate new population using selection, crossover, and mutation
            generateNewPopulation(population, POPULATION_SIZE, next_generation, min_range, max_range);
            
            // Evaluate fitness of new population using coverage
            for (int i = 0; i < POPULATION_SIZE; i++)
            {
                // Reset coverage before evaluating each individual
                __coverage_reset();
                resetCoverageMap(next_generation[i].coverage_map);
                
                // Execute with the mutated input
                executeTargetInt(next_generation[i].input_value, filename);
                
                // Copy coverage data
                if (__coverage_map) {
                    memcpy(next_generation[i].coverage_map, __coverage_map, 
                           COVERAGE_MAP_SIZE * sizeof(coverage_t));
                }
                
                // Calculate fitness based on coverage
                next_generation[i].fitness_score = 
                    calculateCoverageFitness(next_generation[i].coverage_map);
                
                // Check for new coverage
                if (hasNewCoverage(next_generation[i].coverage_map, global_coverage_map)) {
                    updateGlobalCoverage(next_generation[i].coverage_map);
                    
                    // Save to corpus
                    saveToCorpus(next_generation[i].input_value,
                                next_generation[i].coverage_map,
                                next_generation[i].fitness_score,
                                1); // Mark as interesting
                    
                    last_corpus_update = iter;
                }
            }
            
            // Replace current population with the new generation
            memcpy(population, next_generation, sizeof(Individual) * POPULATION_SIZE);
        }
        
        // 3. Occasionally use crossover between corpus entries to create new inputs
        if (getCorpusSize() >= 2 && iter % 5 == 0) {
            // Reset coverage before crossover operations
            __coverage_reset();
            
            // Select two entries from corpus
            CorpusEntry* entry1 = selectCorpusEntry();
            CorpusEntry* entry2 = selectCorpusEntry();
            
            if (entry1 && entry2) {
                // Perform crossover
                int child_input = crossover(entry1->input_value, entry2->input_value);
                
                // Execute with crossover result
                executeTargetInt(child_input, filename);
                
                // Check if the crossover produced interesting results
                coverage_t* child_coverage = calloc(COVERAGE_MAP_SIZE, sizeof(coverage_t));
                if (child_coverage) {
                    memcpy(child_coverage, __coverage_map, COVERAGE_MAP_SIZE * sizeof(coverage_t));
                    
                    double fitness = calculateCoverageFitness(child_coverage);
                    
                    // Check for new coverage
                    if (hasNewCoverage(child_coverage, global_coverage_map)) {
                        updateGlobalCoverage(child_coverage);
                        saveToCorpus(child_input, child_coverage, fitness, 1);
                    }
                    
                    free(child_coverage);
                }
            }
        }
        
        // 4. Occasionally apply havoc mutation to corpus entries
        if (iter % 10 == 0 && getCorpusSize() > 0) {
            // Reset coverage before havoc mutation
            __coverage_reset();
            
            CorpusEntry* entry = selectCorpusEntry();
            
            if (entry) {
                int havoc_input = mutateHavoc(entry->input_value);
                
                // Execute with havoc result
                executeTargetInt(havoc_input, filename);
                
                // Check if the havoc produced interesting results
                coverage_t* havoc_coverage = calloc(COVERAGE_MAP_SIZE, sizeof(coverage_t));
                if (havoc_coverage) {
                    memcpy(havoc_coverage, __coverage_map, COVERAGE_MAP_SIZE * sizeof(coverage_t));
                    
                    double fitness = calculateCoverageFitness(havoc_coverage);
                    
                    // Check for new coverage
                    if (hasNewCoverage(havoc_coverage, global_coverage_map)) {
                        updateGlobalCoverage(havoc_coverage);
                        saveToCorpus(havoc_input, havoc_coverage, fitness, 1);
                    }
                    
                    free(havoc_coverage);
                }
            }
        }
    }
    
    // Save final progress data
    if (progress_file) {
        int covered = __coverage_count();
        fprintf(progress_file, "%d,%d,greybox\n", iterations, covered);
    }
    
    // Print final statistics
    printf("\n=== Grey box fuzzing completed ===\n");
    printf("Total iterations: %d\n", iterations);
    printCorpusStats();
    __coverage_dump();
    
    // Cleanup
    cleanupPopulations();
    cleanupCorpus();
    
    if (progress_file) {
        fclose(progress_file);
    }
}

// Main function with mode selection
int main(int argc, char *argv[])
{
    int opt;
    int random_mode = 0;  // Default is grey box fuzzing
    
    // Parse command line options
    while ((opt = getopt(argc, argv, "r")) != -1) {
        switch (opt) {
            case 'r':
                random_mode = 1;  // Enable random fuzzing mode
                break;
            default:
                fprintf(stderr, "Usage: %s [-r] <filename>\n", argv[0]);
                fprintf(stderr, "  -r    Use random fuzzing instead of grey box\n");
                return 1;
        }
    }
    
    if (optind >= argc) {
        fprintf(stderr, "Expected filename argument after options\n");
        fprintf(stderr, "Usage: %s [-r] <filename>\n", argv[0]);
        return 1;
    }
    
    const char *filename = argv[optind];
    char *temp_path = strdup(filename);
    if (!temp_path) {
        perror("Error duplicating filename");
        return 1;
    }
    
    const char *base_filename = basename(temp_path);
    char *fullPath = realpath(filename, NULL);
    if (!fullPath) {
        perror("Error getting full path");
        free(temp_path);
        return 1;
    }
    
    printf("Full path: %s\n", fullPath);
    printf("Filename: %s\n", filename);
    printf("Base filename: %s\n", base_filename);
    printf("Mode: %s\n", random_mode ? "Random fuzzing" : "Grey box fuzzing");
    
    // Compile target with coverage instrumentation
    printf("Preparing target with coverage instrumentation...\n");
    if (prepareTargetWithCoverage(dirname(strdup(fullPath)), base_filename) != 0) {
        fprintf(stderr, "Coverage instrumentation failed\n");
        // Fall back to regular compilation
        printf("Falling back to regular compilation...\n");
        if (compileTargetFile(filename, base_filename) != 0) {
            fprintf(stderr, "Compilation failed\n");
            free(temp_path);
            free(fullPath);
            return 1;
        }
    }
    
    unsigned int seed = time(NULL);
    printf("Using seed: %u\n", seed);
    srand(seed);
    
    printf("Generating lexer...\n");
    if (generateLexer() != ERR_SUCCESS)
    {
        printf("Failed to generate lexer\n");
        free(temp_path);
        free(fullPath);
        return 1;
    }
    
    printf("Scanning file: %s\n", fullPath);
    if (lexScanFile(fullPath) != ERR_SUCCESS)
    {
        printf("Failed to scan file\n");
        free(temp_path);
        free(fullPath);
        return 1;
    }
    
    struct InputRange range = extractInputRange(OUTPUT_FILE);
    if (!range.valid)
    {
        printf("Failed to extract input range\n");
        free(temp_path);
        free(fullPath);
        return 1;
    }
    
    printf("Input range: [%d, %d]\n", range.min, range.max);
    minRange = range.min;
    maxRange = range.max;
    
    // Initialize coverage tracking
    __coverage_init();
    
    // Reset global coverage map before starting any fuzzing
    resetGlobalCoverageMap();
    
    // Run the selected fuzzing mode
    if (random_mode) {
        randomFuzzing(MAX_ITERATIONS, range.min, range.max, fullPath);
    } else {
        greyBoxFuzzing(MAX_ITERATIONS, range.min, range.max, fullPath);
    }
    
    free(temp_path);
    free(fullPath);
    return 0;
}