#ifndef CORPUS_H
#define CORPUS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "generational.h"

// Structure for corpus testcase
typedef struct {
    int input_value;    // Current implementation only handles integer inputs
    coverage_t* coverage_map;  // Coverage map for this input
    double fitness_score;      // Fitness score for this input
    time_t timestamp;          // When this input was added
    int is_interesting;        // Flag to mark inputs with unique behaviors
} CorpusEntry;

// Corpus management functions
int initializeCorpus(const char* corpus_dir);
int saveToCorpus(int input_value, coverage_t* coverage_map, double fitness_score, int is_interesting);
int loadCorpus(const char* corpus_dir);
int minimizeCorpus();  // Remove redundant inputs
CorpusEntry* selectCorpusEntry();  // Select a promising entry for mutation
void cleanupCorpus(void);

// Statistics
int getCorpusSize(void);
double getAverageCorpusFitness(void);
void printCorpusStats(void);

#endif