#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h> // For read (alternative to fread)
#include <sys/types.h>
#include <fcntl.h>  // Required for O_RDONLY

// --- Vulnerability Functions ---  

// Vulnerability 1: Stack buffer overflow
void trigger_overflow(const uint8_t *data, size_t offset) {
    char local_buffer[32];
    printf("Triggering overflow path...\n");
    // Vulnerability: If data starting at 'offset' is > 31 bytes, this overflows.
    // strcpy is unsafe, use it here intentionally for the vulnerability.
    strcpy(local_buffer, (const char *)(data + offset));
    printf("Overflow survived? Data copied: %s\n", local_buffer); // Might not print if crashed
}

// Vulnerability 2: Null pointer dereference
void trigger_null_deref(int condition) {
    int *ptr = (int *)0xdeadbeef; // Initialize to non-NULL
    printf("Triggering null deref path...\n");
    if (condition) {
        ptr = NULL; // Set to NULL based on some condition
        printf("Pointer set to NULL.\n");
    }
    // Vulnerability: If ptr is NULL, this crashes.
    *ptr = 123;
    printf("Null deref survived? Value set.\n"); // Won't print if crashed
}

// Vulnerability 3: Heap buffer overflow (using read)
void trigger_heap_overflow(int fd, size_t size_to_read) {
    char *heap_buffer = malloc(64);
    if (!heap_buffer) {
        printf("Malloc failed for heap overflow trigger.\n");
        return; // Can't trigger if malloc fails
    }
    printf("Triggering heap overflow path... reading %zu bytes\n", size_to_read);
    // Vulnerability: If size_to_read > 64, this will write past the buffer.
    // Using read() directly can be less common but shows another pattern.
    // Ensure the file descriptor is positioned correctly before calling this.
    ssize_t bytes_read = read(fd, heap_buffer, size_to_read);
    if (bytes_read < 0) {
         perror("Read error in heap trigger");
    } else {
        printf("Heap read %zd bytes.\n", bytes_read);
    }
    // Use the buffer slightly to prevent optimization?
    if (bytes_read > 0 && heap_buffer[0] == 'H') {
        printf("Heap buffer starts with H.\n");
    }
    free(heap_buffer); // Freeing is good practice, but might be after the overflow
}


// --- Main Processing Logic ---
int process_data(const uint8_t *data, size_t size, int fd) {
    printf("Processing data size: %zu\n", size);

    if (size < 8) {
        printf("Input too small.\n");
        return 0; // Not enough data for basic checks
    }

    // Path 1: Check for "FUZZ" magic bytes
    if (memcmp(data, "FUZZ", 4) == 0) {
        printf("Path: FUZZ detected.\n");
        if (size > 10 && data[4] == '!') {
             // Trigger Vuln 1: Stack Overflow
             // Pass data starting after "FUZZ!" (offset 5)
             trigger_overflow(data, 5);
             return 1; // Indicate potential crash path taken
        }
        if (size > 8 && data[4] == '?') {
            printf("Path: FUZZ? variant detected.\n");
            // Some other non-crashing logic here
        }
        return 0;
    }

    // Path 2: Check for "CRASH" magic bytes
    if (size >= 5 && memcmp(data, "CRASH", 5) == 0) {
         printf("Path: CRASH detected.\n");
         // Trigger Vuln 2: Null Pointer Dereference
         // Condition for NULL is if the byte after "CRASH" is 0xFF
         trigger_null_deref(size > 5 && data[5] == 0xFF);
         return 1; // Indicate potential crash path taken
    }

    // Path 3: Check for "HEAP" magic bytes
    if (size >= 4 && memcmp(data, "HEAP", 4) == 0) {
        printf("Path: HEAP detected.\n");
        if (size >= 6) {
            // Use bytes 4 and 5 as a size for a subsequent read
            // This is contrived, usually size comes from parsed data
            uint16_t read_size = 0;
            memcpy(&read_size, data + 4, 2); // Read 2 bytes into size
            printf("HEAP path wants to read %u bytes.\n", read_size);

            // Reposition file descriptor to read *after* the size field
            // Assumes fd is still valid and open for reading
            lseek(fd, 6, SEEK_SET); // Seek past "HEAP" + size bytes

            // Trigger Vuln 3: Heap Overflow (read)
            trigger_heap_overflow(fd, read_size);
            return 1; // Indicate potential crash path taken
        }
        return 0;
    }

    // Default Path
    printf("Path: Default processing.\n");
    // Check a specific byte deep in the input for another path
    if (size > 20 && data[19] == 0xAB) {
        printf("Path: Deep byte check triggered.\n");
    }

    return 0; // Normal exit
}


// --- Main Entry Point ---
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    uint8_t *buffer = NULL;
    long file_size = -1;
    int result = 1; // Default to error exit code
    int fd = -1;    // File descriptor

    // Use file descriptor for potential read() later
    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open input file");
        return 1;
    }

    // Get file size
    off_t current_pos = lseek(fd, 0, SEEK_CUR); // Get current position (should be 0)
    file_size = lseek(fd, 0, SEEK_END);         // Seek to end to get size
    lseek(fd, current_pos, SEEK_SET);           // Seek back to original position

    if (file_size < 0) {
        perror("Failed to get file size (lseek)");
        close(fd);
        return 1;
    }
    if (file_size == 0) {
        printf("Input file is empty.\n");
        close(fd);
        return 0; // Empty file is not an error, just nothing to process
    }
     if (file_size > (1024 * 1024)) { // Limit input size (e.g., 1MB)
        fprintf(stderr, "Input file too large (%ld bytes).\n", file_size);
        close(fd);
        return 1;
    }


    // Allocate buffer and read file content
    buffer = (uint8_t *)malloc(file_size);
    if (!buffer) {
        perror("Failed to allocate buffer");
        close(fd);
        return 1;
    }

    ssize_t bytes_read = read(fd, buffer, file_size);
     if (bytes_read < 0) {
        perror("Failed to read file");
        free(buffer);
        close(fd);
        return 1;
    }
     if (bytes_read != file_size) {
         fprintf(stderr, "Could not read entire file (%zd / %ld bytes).\n", bytes_read, file_size);
         free(buffer);
         close(fd);
         return 1;
     }


    // Process the data
    printf("--- Target started processing: %s ---\n", filename);
    result = process_data(buffer, (size_t)file_size, fd);
    printf("--- Target finished processing ---\n");

    // Cleanup
    free(buffer);
    close(fd); // Close the file descriptor

    // Return 0 normally, non-zero could indicate internal errors
    // or paths taken, though fuzzers usually care about crashes/signals.
    return result;
}