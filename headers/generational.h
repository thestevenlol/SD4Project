#ifndef GENERATIONAL_H
#define GENERATIONAL_H

#define POPULATION_SIZE 10
#define NUM_GENERATIONS 4
#define MUTATION_RATE 0.8      // Probability of mutation (e.g., 80%)
#define TOURNAMENT_SIZE 3

// Coverage map definitions
#define COVERAGE_MAP_SIZE 65536 // 64KB coverage map
typedef unsigned char coverage_t;

typedef struct {
    int input_value;
    double fitness_score; 
    coverage_t* coverage_map; // Coverage map for this individual
} Individual;

// Global population arrays
extern Individual* population;
extern Individual* next_generation;

// Global coverage map to track overall coverage
extern coverage_t* global_coverage_map;

// Function declarations
double calculateCoverageFitness(coverage_t* coverage_map);
void resetCoverageMap(coverage_t* map);
void resetGlobalCoverageMap(void);
int compareCoverageMaps(coverage_t* map1, coverage_t* map2);
int hasNewCoverage(coverage_t* individual_map, coverage_t* global_map);
void updateGlobalCoverage(coverage_t* individual_map);

double calculatePlaceholderFitness(int input_value, int min_range, int max_range);
Individual selectParent(Individual population[], int population_size);
int mutateInteger(int original_value, int min_range, int max_range);
int getNextPopulationIndex(int population_size);
void generateNewPopulation(Individual population[], int population_size, Individual next_generation[], int min_range, int max_range);
void initializePopulations(void);
void cleanupPopulations(void);

#endif