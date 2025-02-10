#include <stdlib.h>

#include "../headers/generational.h"

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

int mutateInteger(int original_value, int min_range, int max_range) {
    // Example mutation: Add or subtract a small random value
    int mutation_amount = (rand() % 10) - 5; // Random value between -5 and 4
    int mutated_value = original_value + mutation_amount;

    // Ensure mutated value stays within range (you might need to adjust range enforcement)
    if (mutated_value < min_range) mutated_value = min_range;
    if (mutated_value > max_range) mutated_value = max_range;

    return mutated_value;
}

void generateNewPopulation(Individual population[], int population_size, Individual next_generation[], int min_range, int max_range) {
    for (int i = 0; i < population_size; i++) {
        Individual parent = selectParent(population, population_size); // Select a parent

        next_generation[i] = parent; // Start with parent's genes (input value and fitness)

        if ((double)rand() / RAND_MAX < MUTATION_RATE) {
            // Apply mutation with a certain probability
            next_generation[i].input_value = mutateInteger(parent.input_value, min_range, max_range);
            next_generation[i].fitness_score = 0.0; // Reset fitness for re-evaluation in next generation
        }
    }
}