#ifndef GENERATIONAL_H
#define GENERATIONAL_H

#define POPULATION_SIZE 100    
#define NUM_GENERATIONS 20     
#define MUTATION_RATE 0.8      // Probability of mutation (e.g., 80%)
#define TOURNAMENT_SIZE 3

typedef struct {
    int input_value;
    double fitness_score; 
} Individual;

double calculatePlaceholderFitness(int input_value, int min_range, int max_range);
Individual selectParent(Individual population[], int population_size);
int mutateInteger(int original_value, int min_range, int max_range);
void generateNewPopulation(Individual population[], int population_size, Individual next_generation[], int min_range, int max_range);

#endif