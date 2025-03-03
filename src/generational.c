#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../headers/generational.h"
#include "../headers/fuzz.h"

// Global population arrays
Individual* population = NULL;
Individual* next_generation = NULL;
coverage_t* global_coverage_map = NULL;
int populationIndex = 0;

void initializePopulations(void) {
    population = malloc(POPULATION_SIZE * sizeof(Individual));
    next_generation = malloc(POPULATION_SIZE * sizeof(Individual));
    global_coverage_map = calloc(COVERAGE_MAP_SIZE, sizeof(coverage_t));
    
    if (!population || !next_generation || !global_coverage_map) {
        fprintf(stderr, "Failed to allocate memory for populations or coverage maps\n");
        exit(1);
    }

    // Initialize coverage maps for each individual
    for (int i = 0; i < POPULATION_SIZE; i++) {
        population[i].coverage_map = calloc(COVERAGE_MAP_SIZE, sizeof(coverage_t));
        next_generation[i].coverage_map = calloc(COVERAGE_MAP_SIZE, sizeof(coverage_t));
        
        if (!population[i].coverage_map || !next_generation[i].coverage_map) {
            fprintf(stderr, "Failed to allocate memory for individual coverage maps\n");
            exit(1);
        }
    }

    populationIndex = 0;
}

int getNextPopulationIndex(int population_size) {
    int index = populationIndex;
    populationIndex = (populationIndex + 1) % population_size;
    return index;
}

void cleanupPopulations(void) {
    if (population) {
        for (int i = 0; i < POPULATION_SIZE; i++) {
            free(population[i].coverage_map);
        }
        free(population);
        population = NULL;
    }
    
    if (next_generation) {
        for (int i = 0; i < POPULATION_SIZE; i++) {
            free(next_generation[i].coverage_map);
        }
        free(next_generation);
        next_generation = NULL;
    }
    
    free(global_coverage_map);
    global_coverage_map = NULL;
}

void resetCoverageMap(coverage_t* map) {
    memset(map, 0, COVERAGE_MAP_SIZE * sizeof(coverage_t));
}

// Reset the global coverage map
void resetGlobalCoverageMap(void) {
    if (global_coverage_map) {
        memset(global_coverage_map, 0, COVERAGE_MAP_SIZE * sizeof(coverage_t));
    }
}

int compareCoverageMaps(coverage_t* map1, coverage_t* map2) {
    return memcmp(map1, map2, COVERAGE_MAP_SIZE * sizeof(coverage_t));
}

int hasNewCoverage(coverage_t* individual_map, coverage_t* global_map) {
    for (int i = 0; i < COVERAGE_MAP_SIZE; i++) {
        if ((individual_map[i] > 0) && (global_map[i] == 0)) {
            return 1;  // Found new coverage
        }
    }
    return 0;
}

void updateGlobalCoverage(coverage_t* individual_map) {
    for (int i = 0; i < COVERAGE_MAP_SIZE; i++) {
        if (individual_map[i] > 0) {
            global_coverage_map[i] = 1;  // Mark as covered in global map
        }
    }
}

// Replace placeholder fitness with coverage-based fitness
double calculateCoverageFitness(coverage_t* coverage_map) {
    // Count total edges covered
    int covered_edges = 0;
    int new_edges = 0;
    
    for (int i = 0; i < COVERAGE_MAP_SIZE; i++) {
        if (coverage_map[i] > 0) {
            covered_edges++;
            if (global_coverage_map[i] == 0) {
                new_edges++;
            }
        }
    }
    
    // Base fitness on coverage with bonus for new edges
    double fitness = (double)covered_edges;
    
    // Heavily reward new edge discovery
    if (new_edges > 0) {
        fitness += new_edges * 10.0;  // Bonus for new edges
    }
    
    return fitness;
}

double calculatePlaceholderFitness(int input_value, int min_range, int max_range) {
    // Example: Higher fitness if input is further from the min/max boundaries
    double range_size = (double)(max_range - min_range);
    if (range_size == 0) return 0.5; // Avoid division by zero

    double normalized_value;
    if (input_value < min_range) {
        normalized_value = (double)(min_range - input_value) / range_size;
    } else if (input_value > max_range) {
        normalized_value = (double)(input_value - max_range) / range_size;
    } else {
        normalized_value = 0.0; // Input within range, lower placeholder fitness
    }
    return 1.0 / (1.0 + normalized_value); // Invert and scale to get higher fitness for "different" inputs
}

// Tournament selection function
Individual selectParent(Individual population[], int population_size) {
    Individual best_individual = population[rand() % population_size]; // Randomly select first tournament member
    for (int i = 1; i < TOURNAMENT_SIZE; i++) {
        Individual current_individual = population[rand() % population_size];
        if (current_individual.fitness_score > best_individual.fitness_score) {
            best_individual = current_individual;
        }
    }
    return best_individual; // Return the fittest individual from the tournament
}

void generateNewPopulation(Individual population[], int population_size, Individual next_generation[], int min_range, int max_range) {
    for (int i = 0; i < population_size; i++) {
        Individual parent = selectParent(population, population_size); // Select a parent

        next_generation[i] = parent; // Start with parent's genes (input value and fitness)

        // Reset coverage map for the new individual
        resetCoverageMap(next_generation[i].coverage_map);

        if ((double)rand() / RAND_MAX < MUTATION_RATE) {
            // Apply mutation with a certain probability
            next_generation[i].input_value = mutateInteger(parent.input_value, min_range, max_range);
            next_generation[i].fitness_score = 0.0; // Reset fitness for re-evaluation in next generation
        }
    }
}