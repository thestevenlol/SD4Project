// filepath: headers/generational.h
#ifndef GENERATIONAL_H
#define GENERATIONAL_H

#include <time.h>
#include "coverage.h" // Include coverage.h to get coverage_t and COVERAGE_MAP_SIZE
#include "corpus.h" // For TestCase

// --- Configuration ---
#define POPULATION_SIZE 100     // Number of individuals in the population
#define TOURNAMENT_SIZE 5       // Size of selection tournament
#define MUTATION_RATE 0.15      // Probability of mutation (adjust as needed)
#define CROSSOVER_RATE 0.7     // Probability of crossover (adjust as needed)
#define NUM_GENERATIONS 5       // Number of generations per main fuzzer iteration

// Define if the Individual struct's coverage_map is a pointer vs array
// If it's coverage_t* coverage_map; then define this
#define INDIVIDUAL_MAP_IS_POINTER

// Structure for an individual in the population
typedef struct {
    int input_value;            // The input (genome)
    double fitness_score;       // Fitness score (e.g., based on coverage)
    time_t timestamp;           // Time when created/found

#ifdef INDIVIDUAL_MAP_IS_POINTER
    coverage_t* coverage_map;   // Pointer to coverage map for this individual's execution
#else
    coverage_t coverage_map[COVERAGE_MAP_SIZE]; // Array for coverage map
#endif

} Individual;

// --- Population Management ---
void initializePopulations(void);
void cleanupPopulations(void);

// --- GA Operations ---
Individual selectParent(Individual population[], int population_size);
void generateNewPopulation(Individual population[], int population_size, Individual next_generation[], int min_range, int max_range);

// --- Utility Functions for Individual Maps ---
void resetIndividualCoverageMap(coverage_t* map);
// **FIX:** Add const to match definition in generational.c
int compareCoverageMaps(const coverage_t* map1, const coverage_t* map2);

// --- Byte-level GA Operators ---
// One-point crossover for TestCase data
void tc_crossover(uint8_t *out, TestCase *p1, TestCase *p2);
// Mutate a buffer of given length
void tc_mutate(uint8_t *buf, size_t len);
// Create offspring TestCase by selecting parents, crossover and mutation
void make_offspring(TestCase *child);

// --- Global Variables (Consider encapsulating or passing as params) ---
extern Individual* population;
extern Individual* next_generation;
// extern coverage_t* global_coverage_map; // Now managed in main.c


#endif // GENERATIONAL_H