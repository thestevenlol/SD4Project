#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include "../headers/corpus.h"
#include "../headers/uthash.h"  // Using uthash for efficient corpus management

// Hash table entry for uthash
typedef struct {
    int input_value;           // key
    CorpusEntry* entry;        // value
    UT_hash_handle hh;         // makes this structure hashable
} CorpusHash;

// Global corpus variables
static CorpusHash* corpus_table = NULL;
static int corpus_size = 0;
static char corpus_directory[1024] = {0};

int initializeCorpus(const char* corpus_dir) {
    // Clean up any previous corpus
    cleanupCorpus();
    
    // Store corpus directory
    strncpy(corpus_directory, corpus_dir, sizeof(corpus_directory) - 1);
    
    // Create directory if it doesn't exist
    struct stat st = {0};
    if (stat(corpus_directory, &st) == -1) {
        if (mkdir(corpus_directory, 0700) == -1) {
            fprintf(stderr, "Failed to create corpus directory: %s\n", corpus_directory);
            return -1;
        }
    }
    
    return 0;
}

int saveToCorpus(int input_value, coverage_t* coverage_map, double fitness_score, int is_interesting) {
    CorpusHash* entry;
    
    // Check if input already exists in corpus
    HASH_FIND_INT(corpus_table, &input_value, entry);
    if (entry) {
        // Update existing entry if this one is more interesting
        if (fitness_score > entry->entry->fitness_score) {
            entry->entry->fitness_score = fitness_score;
            memcpy(entry->entry->coverage_map, coverage_map, COVERAGE_MAP_SIZE * sizeof(coverage_t));
            entry->entry->is_interesting = is_interesting;
            entry->entry->timestamp = time(NULL);
        }
    } else {
        // Create new entry
        entry = (CorpusHash*)malloc(sizeof(CorpusHash));
        if (!entry) {
            fprintf(stderr, "Failed to allocate memory for corpus hash entry\n");
            return -1;
        }
        
        entry->input_value = input_value;
        entry->entry = (CorpusEntry*)malloc(sizeof(CorpusEntry));
        if (!entry->entry) {
            fprintf(stderr, "Failed to allocate memory for corpus entry\n");
            free(entry);
            return -1;
        }
        
        entry->entry->input_value = input_value;
        entry->entry->coverage_map = (coverage_t*)malloc(COVERAGE_MAP_SIZE * sizeof(coverage_t));
        if (!entry->entry->coverage_map) {
            fprintf(stderr, "Failed to allocate memory for coverage map\n");
            free(entry->entry);
            free(entry);
            return -1;
        }
        
        memcpy(entry->entry->coverage_map, coverage_map, COVERAGE_MAP_SIZE * sizeof(coverage_t));
        entry->entry->fitness_score = fitness_score;
        entry->entry->is_interesting = is_interesting;
        entry->entry->timestamp = time(NULL);
        
        // Add to hash table
        HASH_ADD_INT(corpus_table, input_value, entry);
        corpus_size++;
        
        // Save to file
        char filename[1024];
        snprintf(filename, sizeof(filename), "%s/input-%d", corpus_directory, input_value);
        FILE* fp = fopen(filename, "w");
        if (fp) {
            fprintf(fp, "%d\n", input_value);
            fclose(fp);
        }
    }
    
    return 0;
}

int loadCorpus(const char* corpus_dir) {
    DIR* dir;
    struct dirent* entry;
    char filepath[1024];
    FILE* fp;
    int input_value;
    
    // Set corpus directory
    strncpy(corpus_directory, corpus_dir, sizeof(corpus_directory) - 1);
    
    // Open directory
    dir = opendir(corpus_directory);
    if (!dir) {
        fprintf(stderr, "Failed to open corpus directory: %s\n", corpus_directory);
        return -1;
    }
    
    // Read each file in the directory
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "input-", 6) == 0) {
            snprintf(filepath, sizeof(filepath), "%s/%s", corpus_directory, entry->d_name);
            fp = fopen(filepath, "r");
            if (fp) {
                if (fscanf(fp, "%d", &input_value) == 1) {
                    // Initialize coverage map for this input (we can't load coverage from file yet)
                    coverage_t* coverage_map = calloc(COVERAGE_MAP_SIZE, sizeof(coverage_t));
                    if (coverage_map) {
                        // Placeholder fitness score until we re-evaluate
                        saveToCorpus(input_value, coverage_map, 0.0, 0);
                        free(coverage_map);
                    }
                }
                fclose(fp);
            }
        }
    }
    
    closedir(dir);
    return corpus_size;
}

int minimizeCorpus() {
    if (corpus_size <= 1) {
        return corpus_size; // Nothing to minimize
    }
    
    // First pass: identify unique coverage contributions
    CorpusHash* current, *tmp;
    int redundant_count = 0;
    
    // Create a global coverage map to track what's covered by all inputs
    coverage_t* global_coverage = calloc(COVERAGE_MAP_SIZE, sizeof(coverage_t));
    if (!global_coverage) {
        return -1;
    }
    
    // Collect all unique coverage first
    HASH_ITER(hh, corpus_table, current, tmp) {
        for (int i = 0; i < COVERAGE_MAP_SIZE; i++) {
            if (current->entry->coverage_map[i] > 0) {
                global_coverage[i] = 1;
            }
        }
    }
    
    // Find redundant entries
    HASH_ITER(hh, corpus_table, current, tmp) {
        int is_redundant = 1;
        
        // Skip entries marked as interesting regardless of coverage
        if (current->entry->is_interesting) {
            continue;
        }
        
        // Check if this entry contributes any unique coverage
        for (int i = 0; i < COVERAGE_MAP_SIZE; i++) {
            if (current->entry->coverage_map[i] > 0) {
                // See if any other entry covers this position
                int covered_elsewhere = 0;
                CorpusHash* other;
                
                HASH_ITER(hh, corpus_table, other, tmp) {
                    if (other != current && other->entry->coverage_map[i] > 0) {
                        covered_elsewhere = 1;
                        break;
                    }
                }
                
                if (!covered_elsewhere) {
                    is_redundant = 0; // This entry contributes unique coverage
                    break;
                }
            }
        }
        
        if (is_redundant) {
            // Remove this entry
            HASH_DEL(corpus_table, current);
            free(current->entry->coverage_map);
            free(current->entry);
            free(current);
            corpus_size--;
            redundant_count++;
        }
    }
    
    free(global_coverage);
    
    printf("Corpus minimized: removed %d redundant entries\n", redundant_count);
    return corpus_size;
}

CorpusEntry* selectCorpusEntry() {
    if (corpus_size == 0) {
        return NULL;
    }
    
    // Simple strategy: randomly select an entry
    int index = rand() % corpus_size;
    
    // Traverse the hash to get the nth element
    CorpusHash* current;
    int i = 0;
    
    for (current = corpus_table; current != NULL && i < index; current = current->hh.next) {
        i++;
    }
    
    return current ? current->entry : NULL;
}

void cleanupCorpus(void) {
    CorpusHash* current, *tmp;
    
    HASH_ITER(hh, corpus_table, current, tmp) {
        HASH_DEL(corpus_table, current);
        free(current->entry->coverage_map);
        free(current->entry);
        free(current);
    }
    
    corpus_table = NULL;
    corpus_size = 0;
}

int getCorpusSize(void) {
    return corpus_size;
}

double getAverageCorpusFitness(void) {
    if (corpus_size == 0) {
        return 0.0;
    }
    
    double sum = 0.0;
    CorpusHash* current, *tmp;
    
    HASH_ITER(hh, corpus_table, current, tmp) {
        sum += current->entry->fitness_score;
    }
    
    return sum / corpus_size;
}

void printCorpusStats(void) {
    printf("Corpus Statistics:\n");
    printf("  Total entries: %d\n", corpus_size);
    printf("  Average fitness: %.2f\n", getAverageCorpusFitness());
    
    // Count interesting entries
    int interesting = 0;
    CorpusHash* current, *tmp;
    
    HASH_ITER(hh, corpus_table, current, tmp) {
        if (current->entry->is_interesting) {
            interesting++;
        }
    }
    
    printf("  Interesting entries: %d\n", interesting);
}
