#include <stdio.h>

/**
 * @brief Opens a file in append and read mode.
 * 
 * This function attempts to open the specified file in "a+" mode,
 * which allows for both reading and appending to the file.
 * If the file doesn't exist, it will be created.
 * 
 * @param filename The name/path of the file to be opened
 * @return FILE* A pointer to the opened file, or NULL if the operation fails
 * 
 * @note The caller is responsible for closing the file when done
 * @see fclose()
 */
FILE* openFile(const char* filename) {
    FILE* file = fopen(filename, "a+");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }
    return file;
}

/**
 * Writes a string to a file stream and flushes the buffer.
 * 
 * @param file   Pointer to the FILE stream to write to
 * @param string Pointer to the null-terminated string to write
 * 
 * @return 1 on success, 0 if:
 *         - file pointer is NULL
 *         - string pointer is NULL 
 *         - writing to file fails
 *         - flushing the file buffer fails
 */
int writeStringToFile(FILE* file, const char* string) {
    if (file == NULL || string == NULL) {
        return 0;
    }
    
    if (fputs(string, file) == EOF) {
        return 0;
    }
    
    if (fflush(file) != 0) {
        return 0;
    }
    
    return 1;
}

/**
 * @brief Closes a file stream safely
 * 
 * This function attempts to close a file stream and performs error checking.
 * It verifies if the file pointer is valid and ensures the file is closed properly.
 * 
 * @param file Pointer to the FILE stream to be closed
 * @return int Returns 1 on successful closure, 0 on failure or if file is NULL
 */
int closeFile(FILE* file) {
    if (file == NULL) {
        return 0;
    }
    if (fclose(file) != 0) {
        return 0;
    }
    return 1;
}