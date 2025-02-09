#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <libgen.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>

#include "headers/fuzz.h"
#include "headers/lex.h"
#include "headers/io.h"
#include "headers/testcase.h"
#include "headers/target.h"
#include "headers/range.h"
#include "headers/generational.h"
#include "headers/coverage.h"
#include "headers/logger.h"

#define BATCH_SIZE 100000
#define N_TESTS 20

int main(int argc, char *argv[])
{
    app_log(LOG_INFO, "Starting fuzzer application");

    if (argc != 2)
    {
        app_log(LOG_ERROR, "Invalid number of arguments");
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    char *temp_path = strdup(filename);
    const char *base_filename = basename(temp_path);

    app_log_with_value(LOG_INFO, "Processing input file:", "%s", filename);

    char *fullPath = realpath(filename, NULL);
    if (!fullPath)
    {
        app_log(LOG_ERROR, "Failed to resolve full path");
        perror("Error getting full path");
        return 1;
    }

    app_log_with_value(LOG_DEBUG, "Full path resolved:", "%s", fullPath);
    app_log_with_value(LOG_DEBUG, "Base filename:", "%s", base_filename);

    // Compile target with coverage enabled
    app_log(LOG_INFO, "Compiling target file with coverage instrumentation");
    if (compileTargetFile(filename, base_filename) != 0) {
        app_log(LOG_ERROR, "Compilation failed");
        fprintf(stderr, "Compilation failed\n");
        return 1;
    }
    app_log(LOG_INFO, "Target file compilation successful");

    unsigned int seed = time(NULL);
    app_log_with_value(LOG_INFO, "Initializing random seed:", "%u", seed);
    srand(seed);

    app_log(LOG_INFO, "Generating lexer");
    if (generateLexer() != ERR_SUCCESS)
    {
        app_log(LOG_ERROR, "Failed to generate lexer");
        return 1;
    }
    app_log(LOG_INFO, "Lexer generated successfully");

    app_log_with_value(LOG_INFO, "Scanning file:", "%s", fullPath);
    if (lexScanFile(fullPath) != ERR_SUCCESS)
    {
        app_log(LOG_ERROR, "Failed to scan file");
        return 1;
    }
    app_log(LOG_INFO, "File scan completed successfully");

    struct InputRange range = extractInputRange(OUTPUT_FILE);
    if (!range.valid)
    {
        app_log(LOG_ERROR, "Failed to extract input range");
        return 1;
    }

    app_log_with_value(LOG_INFO, "Input range extracted:", "[%d, %d]", range.min, range.max);
    minRange = range.min;
    maxRange = range.max;

    // --- Generational Algorithm Implementation Starts Here ---
    app_log(LOG_INFO, "Starting generational algorithm");

    Individual population[POPULATION_SIZE];
    Individual next_generation[POPULATION_SIZE];

    // 1. Initialize Population
    app_log(LOG_INFO, "Initializing population");
    for (int i = 0; i < POPULATION_SIZE; i++)
    {
        population[i].input_value = generateRandomNumber();
        population[i].fitness_score = 0.0;
        app_log_with_value(LOG_DEBUG, "Generated individual:", "ID: %d, Value: %d", i, population[i].input_value);
    }

    // 2. Generational Loop
    for (int generation = 0; generation < NUM_GENERATIONS; generation++)
    {
        app_log_with_value(LOG_INFO, "Starting generation:", "%d/%d", generation + 1, NUM_GENERATIONS);

        // a) Evaluate Fitness using coverage score
        app_log(LOG_INFO, "Evaluating population fitness");
        for (int i = 0; i < POPULATION_SIZE; i++)
        {
            app_log_with_value(LOG_DEBUG, "Evaluating individual:", "ID: %d, Value: %d", i, population[i].input_value);
            
            executeTargetInt(population[i].input_value);

            char gcov_command[512];
            // Fix: Use base_filename instead of fullPath for gcov
            snprintf(gcov_command, sizeof(gcov_command), "gcov target.c");
            app_log_with_value(LOG_DEBUG, "Running gcov command:", "%s", gcov_command);
            
            system(gcov_command);
            
            // Fix: Look for target.c.gcov instead of full path
            FILE *test_fp = fopen("target.c.gcov", "r");
            if (!test_fp) {
                app_log_with_value(LOG_ERROR, "gcov file not found:", "File: target.c.gcov, Input: %d", population[i].input_value);
                population[i].fitness_score = 0.0;
            } else {
                fclose(test_fp);
                GcovCoverageData cov = parseGcovFile("target.c.gcov");
                if (cov.executed_lines + cov.not_executed_lines > 0)
                    population[i].fitness_score = (double) cov.executed_lines / 
                        (cov.executed_lines + cov.not_executed_lines) * 100.0;
                else
                    population[i].fitness_score = 0.0;
                
                app_log_with_value(LOG_DEBUG, "Coverage data:", "Input: %d, Coverage: %.2f%%", 
                              population[i].input_value, population[i].fitness_score);
            }

            FILE *cov_file = fopen("temp_coverage.txt", "a");
            if (cov_file) {
                fprintf(cov_file, "Input: %d, Coverage: %.2f%%\n", 
                        population[i].input_value, population[i].fitness_score);
                fclose(cov_file);
            }
        }

        app_log(LOG_INFO, "Generating new population");
        generateNewPopulation(population, POPULATION_SIZE, next_generation, minRange, maxRange);

        memcpy(population, next_generation, sizeof(population));
        app_log(LOG_INFO, "Population updated for next generation");
    }

    app_log(LOG_INFO, "Fuzzing process completed");
    free(fullPath);

    return 0;
}