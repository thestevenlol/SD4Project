// filepath: src/generational.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> // For uint8_t
#include "../headers/generational.h"
#include "../headers/fuzz.h"
#include "../headers/coverage.h" // For COVERAGE_MAP_SIZE, coverage_t, calculate_coverage_fitness
#include "../headers/corpus.h"

// Global population arrays (assuming these are defined/sized in generational.h or elsewhere)
Individual *population = NULL;
Individual *next_generation = NULL;
// coverage_t* global_coverage_map = NULL; // Now managed in main.c
int populationIndex = 0; // Use static if only used here?

// Initialize memory for populations and their coverage maps
void initializePopulations(void)
{
    population = malloc(POPULATION_SIZE * sizeof(Individual));
    next_generation = malloc(POPULATION_SIZE * sizeof(Individual));
    // global_coverage_map = calloc(COVERAGE_MAP_SIZE, sizeof(coverage_t)); // Moved to main

    if (!population || !next_generation)
    {
        fprintf(stderr, "Failed to allocate memory for populations\n");
        exit(1);
    }

    // Initialize coverage maps for each individual within the population structures
    for (int i = 0; i < POPULATION_SIZE; i++)
    {
// Assuming Individual struct has 'coverage_map' field defined as:
// coverage_t coverage_map[COVERAGE_MAP_SIZE]; OR coverage_t* coverage_map;
#ifdef INDIVIDUAL_MAP_IS_POINTER
        // If it's a pointer, allocate memory for each map
        population[i].coverage_map = calloc(COVERAGE_MAP_SIZE, sizeof(coverage_t));
        next_generation[i].coverage_map = calloc(COVERAGE_MAP_SIZE, sizeof(coverage_t));
        if (!population[i].coverage_map || !next_generation[i].coverage_map)
        {
            fprintf(stderr, "Failed to allocate memory for individual coverage maps\n");
            // Need better cleanup here
            exit(1);
        }
#else
        // If coverage_map is an array member, just zero it out (implicitly done by calloc if struct is calloc'd)
        memset(population[i].coverage_map, 0, COVERAGE_MAP_SIZE * sizeof(coverage_t));
        memset(next_generation[i].coverage_map, 0, COVERAGE_MAP_SIZE * sizeof(coverage_t));
#endif
    }

    populationIndex = 0;
}

// Cleanup memory allocated for populations
void cleanupPopulations(void)
{
#ifdef INDIVIDUAL_MAP_IS_POINTER
    // If maps were dynamically allocated, free them
    if (population)
    {
        for (int i = 0; i < POPULATION_SIZE; i++)
        {
            free(population[i].coverage_map);
        }
    }
    if (next_generation)
    {
        for (int i = 0; i < POPULATION_SIZE; i++)
        {
            free(next_generation[i].coverage_map);
        }
    }
#endif

    free(population);
    population = NULL;
    free(next_generation);
    next_generation = NULL;
    // free(global_coverage_map); // Moved to main
    // global_coverage_map = NULL;
}

// --- Functions operating on individual coverage maps ---

// Reset an individual's coverage map
void resetIndividualCoverageMap(coverage_t *map)
{
    if (map)
    {
        memset(map, 0, COVERAGE_MAP_SIZE * sizeof(coverage_t));
    }
}

// Compare two individual coverage maps (e.g., for uniqueness check)
int compareCoverageMaps(const coverage_t *map1, const coverage_t *map2)
{
    if (!map1 || !map2)
        return -1; // Or indicate error differently
    return memcmp(map1, map2, COVERAGE_MAP_SIZE * sizeof(coverage_t));
}

// --- GA Specific Functions ---

// Tournament selection function (unchanged conceptually)
Individual selectParent(Individual population[], int population_size)
{
    // Ensure tournament size is valid
    int tournament_size = (TOURNAMENT_SIZE > population_size) ? population_size : TOURNAMENT_SIZE;
    if (tournament_size <= 0)
        return population[0]; // Should not happen

    // Select the first contender randomly
    Individual best_individual = population[rand() % population_size];

    // Run the tournament
    for (int i = 1; i < tournament_size; i++)
    {
        Individual current_individual = population[rand() % population_size];
        // Compare fitness scores
        if (current_individual.fitness_score > best_individual.fitness_score)
        {
            best_individual = current_individual; // Found a better contender
        }
    }
    return best_individual; // Return the fittest from the tournament
}

// Generate the next generation based on selection, crossover, and mutation
void generateNewPopulation(Individual population[], int population_size, Individual next_generation[], int min_range, int max_range) {
    for (int i = 0; i < population_size; i++) {

        // Selection
        Individual parent1 = selectParent(population, population_size);

        // --- Initialize the child struct ---
        Individual child;
        child.input_value = 0; // Default
        child.fitness_score = 0.0;
        child.timestamp = time(NULL); // Set timestamp
#ifdef INDIVIDUAL_MAP_IS_POINTER
        child.coverage_map = NULL; // *** THIS LINE IS CRUCIAL AND WAS MISSING ***
#endif

        // --- Crossover or Mutation ---
        if ((double)rand() / RAND_MAX < CROSSOVER_RATE && population_size >= 2) {
             Individual parent2 = selectParent(population, population_size);
             child.input_value = crossover(parent1.input_value, parent2.input_value);
        } else {
             child.input_value = mutateInteger(parent1.input_value, min_range, max_range);
        }

        // --- Clamp input ---
        if (child.input_value < min_range) child.input_value = min_range;
        if (child.input_value > max_range) child.input_value = max_range;

        // --- Reset fitness (redundant if initialized above, but harmless) ---
        child.fitness_score = 0.0;

        // --- Allocate and Reset Map ---
#ifdef INDIVIDUAL_MAP_IS_POINTER
        // Allocate map for the child *if it doesn't have one* (check is now reliable)
        if (!child.coverage_map) {
             child.coverage_map = calloc(COVERAGE_MAP_SIZE, sizeof(coverage_t));
             if (!child.coverage_map) {
                 // Handle allocation failure
                 fprintf(stderr, "[GA] FATAL: Failed to allocate map for child in generateNewPopulation\n");
                 exit(1);
             }
             // No need to call resetIndividualCoverageMap right after calloc,
             // as calloc already zeroes memory.
        } else {
             // This case shouldn't happen if we just initialized to NULL,
             // but if it did, reset the existing map.
             resetIndividualCoverageMap(child.coverage_map);
        }
        // If you want to be absolutely sure it's zeroed AFTER allocation check:
        // resetIndividualCoverageMap(child.coverage_map); // Call it here if preferred

#else // coverage_map is an array member
        // Just reset the array contents
        resetIndividualCoverageMap(child.coverage_map);
#endif


        // --- Assign child to the next generation ---
        next_generation[i] = child;
    }
    //fprintf(stderr, "[GA] New generation created.\n");
}

// Byte-level one-point crossover for TestCase data
void tc_crossover(uint8_t *out, TestCase *p1, TestCase *p2) {
    size_t len = p1->len < p2->len ? p1->len : p2->len;
    size_t cp = rand() % len;
    memcpy(out, p1->data, cp);
    memcpy(out + cp, p2->data + cp, len - cp);
}

// Mutate a buffer of given length by random byte flips
void tc_mutate(uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        if ((double)rand() / RAND_MAX < MUTATION_RATE) {
            buf[i] = (uint8_t)(rand() % 256);
        }
    }
}

// Create offspring TestCase by selecting parents, crossover and mutation
void make_offspring(TestCase *child) {
    TestCase *p1 = select_parent();
    TestCase *p2 = select_parent();
    if (!p1 || !p2) return;
    // Determine child length based on first parent
    child->len = p1->len;
    // Allocate or reset coverage map
    child->coverage_map = calloc(COVERAGE_MAP_SIZE, sizeof(coverage_t));
    // Generate data
    tc_crossover(child->data, p1, p2);
    // Apply mutation
    tc_mutate(child->data, child->len);
    // Initialize fitness
    child->fitness = 0;
}