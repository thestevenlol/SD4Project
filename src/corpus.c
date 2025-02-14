#include "../headers/corpus.h"

int createCorpus(char* path) {
    // Open file in write mode
    FILE* file = fopen(path, "w");

    // Check if file opened successfully
    if (!file) {
        return 1;
    }

    // Close file
    fclose(file);

    return 0;
}

int selectRandomValueFromCorpus(char* path) {
    // Open file in read mode
    FILE* file = fopen(path, "r");

    // Check if file opened successfully
    if (!file) {
        return -1;
    }

    // Read the number of lines in the file
    int lines = 0;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file)) {
        lines++;
    }

    // Generate a random number between 1 and the number of lines
    int random_line = rand() % lines;

    // Reset file pointer to beginning
    rewind(file);

    // Read the line at the generated random number
    int current_line = 0;
    while (fgets(buffer, sizeof(buffer), file)) {
        if (current_line == random_line) {
            break;
        }
        current_line++;
    }

    // Close file
    fclose(file);
    
    return atoi(buffer);
}

int addValueToCorpus(char* path, int value) {
    // Open file in append mode
    FILE* file = fopen(path, "a");
    
    // Check if file opened successfully
    if (!file) {
        return 1;
    }

    // Write the new value to the file
    fprintf(file, "%d\n", value);

    // Close file
    fclose(file);

    return 0;
}

int removeValueFromCorpus(char* path, int value) {
    // Open file in read mode
    FILE* file = fopen(path, "r");
    
    // Check if file opened successfully
    if (!file) {
        return 1;
    }

    // Open temporary file in write mode
    FILE* temp_file = fopen("temp.txt" , "w");

    // Check if temporary file opened successfully
    if (!temp_file) {
        fclose(file);
        return 1;
    }

    // Copy all lines except the one to be removed to the temporary file
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file)) {
        if (atoi(buffer) != value) {
            fprintf(temp_file, "%s", buffer);
        }
    }

    // Close both files
    fclose(file);
    fclose(temp_file);

    // Remove original file
    remove(path);

    // Rename temporary file to original file
    rename("temp.txt", path);

    return 0;
}
