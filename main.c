#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <libgen.h>
#include <limits.h>

#include "headers/fuzz.h"
#include "headers/lex.h"
#include "headers/io.h"
#include "headers/testcase.h"
#include "headers/target.h"
#include "headers/range.h"
#include "headers/generational.h"
#include "headers/coverage.h"
#include "headers/corpus.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    char *temp_path = strdup(filename);
    if (!temp_path)
    {
        perror("Error duplicating filename");
        return 1;
    }
    const char *base_filename = basename(temp_path);

    char *fullPath = realpath(filename, NULL);
    if (!fullPath)
    {
        perror("Error getting full path");
        return 1;
    }

    printf("Full path: %s\n", fullPath);
    printf("Filename: %s\n", filename);
    printf("Base filename: %s\n", base_filename);
    //createTestSuiteAndMetadata(fullPath, base_filename);
    
    if (createCorpus("corpus.txt") != 0)
    {
        printf("Failed to create corpus\n");
        return 1;
    }

    // Compile target with coverage enabled
    if (compileTargetFile(filename, base_filename) != 0) {
        fprintf(stderr, "Compilation failed\n");
        return 1;
    }

    unsigned int seed = time(NULL);
    printf("Using seed: %u\n", seed);
    srand(seed);

    printf("Generating lexer...\n");
    if (generateLexer() != ERR_SUCCESS)
    {
        printf("Failed to generate lexer\n");
        return 1;
    }

    printf("Scanning file: %s\n", fullPath);
    if (lexScanFile(fullPath) != ERR_SUCCESS)
    {
        printf("Failed to scan file\n");
        return 1;
    }

    struct InputRange range = extractInputRange(OUTPUT_FILE);
    if (!range.valid)
    {
        printf("Failed to extract input range\n");
        return 1;
    }

    printf("Input range: [%d, %d]\n", range.min, range.max);
    minRange = range.min;
    maxRange = range.max;

    // --- Generational Algorithm Implementation Starts Here ---
    initializePopulations();

    // 1. Initialize Population
    printf("Initializing population...\n");
    for (int i = 0; i < POPULATION_SIZE; i++)
    {
        population[i].input_value = generateRandomNumber();
        population[i].fitness_score = 0.0; // Initialize fitness to 0
    }

    // 2. Generational Loop
    for (int generation = 0; generation < NUM_GENERATIONS; generation++)
    {
        printf("\n--- Generation %d ---\n", generation + 1);

        // a Evaluate Fitness using coverage score
        printf("Evaluating fitness using coverage...\n");
        for (int i = 0; i < POPULATION_SIZE; i++)
        {
            executeTargetInt(population[i].input_value);
        }

        // b Generate New Population (Selection, Mutation)
        printf("Generating new population...\n");
        generateNewPopulation(population, POPULATION_SIZE, next_generation, minRange, maxRange);

        // c Replace current population with the new generation
        memcpy(population, next_generation, sizeof(Individual) * POPULATION_SIZE);
        printf("Population updated for next generation.\n");
    }

    cleanupPopulations();
    free(fullPath);

    return 0;
}