#include <stdlib.h>

#include "../headers/io.h"

#define PATH_MAX 4096

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
 * @brief Create a new folder at basePath/folderName
 * 
 * @param basePath The path leading to the folder.
 * @param folderName The name of the folder.
 * @return int - 0 if failure, 1 if success.
 */
int createFolder(const char* basePath, const char* folderName) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", basePath, folderName);
    if (mkdir(path, 0777) == -1) {
        perror("Error creating folder");
        return 0;
    }
    return 1;
}

/**
 * @brief Resolves the absolute path of a given file path.
 *
 * This function takes a relative or absolute file path and returns the
 * absolute path. It uses the `realpath` function to resolve the path.
 *
 * @param filePath The relative or absolute file path to be resolved.
 * @return A pointer to a string containing the absolute path, or NULL if
 *         an error occurs. The caller is responsible for freeing the
 *         returned string.
 */
char* getFullPath(const char* filePath) {
    char* resolved_path = (char*)malloc(PATH_MAX);
    if (resolved_path == NULL) {
        return NULL;
    }

    if (realpath(filePath, resolved_path) == NULL) {
        free(resolved_path);
        return NULL;
    }

    return resolved_path;
}


/**
 * @brief Generates the SHA-256 hash of a file.
 *
 * This function takes a file path as input, executes an external script to
 * generate the SHA-256 hash of the file, and returns the hash as a string.
 *
 * @param filepath The path to the file for which the hash is to be generated.
 * @return A dynamically allocated string containing the SHA-256 hash of the file,
 *         or NULL if an error occurs. The caller is responsible for freeing the
 *         returned string.
 */
char* getHash(const char* filepath) {
    if (!filepath) return NULL;
    
    // Build command
    char command[PATH_MAX + 20];
    snprintf(command, sizeof(command), "./hash.sh \"%s\"", filepath);
    
    // Execute command
    FILE* pipe = popen(command, "r");
    if (!pipe) return NULL;
    
    // Read output
    char* hash = malloc(65); // SHA-256 is 64 chars + null
    if (!hash) {
        pclose(pipe);
        return NULL;
    }
    
    // Skip first line (full path)
    char buffer[PATH_MAX];
    if (!fgets(buffer, sizeof(buffer), pipe)) {
        free(hash);
        pclose(pipe);
        return NULL;
    }
    
    // Read hash
    if (!fgets(hash, 65, pipe)) {
        free(hash);
        pclose(pipe);
        return NULL;
    }
    
    // Remove newline if present
    size_t len = strlen(hash);
    if (len > 0 && hash[len-1] == '\n') {
        hash[len-1] = '\0';
    }
    
    pclose(pipe);
    return hash;
}