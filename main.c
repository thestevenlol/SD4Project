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

#define BATCH_SIZE 100000
#define N_TESTS 20

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    char *temp_path = strdup(filename);
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

    Individual population[POPULATION_SIZE];
    Individual next_generation[POPULATION_SIZE];

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
        printf("\n--- Generation %d ---\n", generation + 1); // Generation numbering starts from 1 for readability

        // a Evaluate Fitness using coverage score
        printf("Evaluating fitness using coverage...\n");
        for (int i = 0; i < POPULATION_SIZE; i++)
        {
            // Execute target with the individual's input
            executeTargetInt(population[i].input_value);

            // Build gcov command using the fullPath variable for source file
            char gcov_command[512];
            char gcov_output[512];  // Fixed: Changed from pointer to array
            printf("\n\n================ MISERY ================\n\n");
            snprintf(gcov_command, sizeof(gcov_command), "gcov %s", fullPath);
            printf("Running gcov command... %s\n", gcov_command);
            printf("\n\n================ MISERY ================\n\n");

            // todo: 
            // 1. Issue with min and max range not being passed to the target program
            // 2. Issue with gcov output file not being generated
            
            // Run gcov command to generate coverage data and .gcov file
            system(gcov_command);
            
            // Check if gcov output file exists before parsing
            snprintf(gcov_output, sizeof(gcov_output), "%s.gcov", fullPath);
            FILE *test_fp = fopen(gcov_output, "r");
            if (!test_fp) {
                fprintf(stderr, "gcov file (%s) not found for input %d\n", gcov_output, population[i].input_value);
                population[i].fitness_score = 0.0;
            } else {
                fclose(test_fp);
                GcovCoverageData cov = parseGcovFile("target.c.gcov");
                if (cov.executed_lines + cov.not_executed_lines > 0)
                    population[i].fitness_score = (double) cov.executed_lines / 
                        (cov.executed_lines + cov.not_executed_lines) * 100.0;
                else
                    population[i].fitness_score = 0.0;
            }
            // Save coverage score to a temporary file
            FILE *cov_file = fopen("temp_coverage.txt", "a");
            if (cov_file) {
                fprintf(cov_file, "Input: %d, Coverage: %.2f%%\n", 
                        population[i].input_value, population[i].fitness_score);
                fclose(cov_file);
            }
            printf("  Input: %d, Coverage: %.2f%%\n", 
                   population[i].input_value, population[i].fitness_score);
        }

        // b Generate New Population (Selection, Mutation)
        printf("Generating new population...\n");
        generateNewPopulation(population, POPULATION_SIZE, next_generation, minRange, maxRange);

        // c Replace current population with the new generation
        memcpy(population, next_generation, sizeof(population));
        printf("Population updated for next generation.\n");
    }

    free(fullPath); // Now free fullPath after all uses.

    return 0;
}