#ifndef SIGNALS_H
#define SIGNALS_H
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ucontext.h>
#include <execinfo.h>
#include <unistd.h>
#include "uthash.h"
#include "coverage.h"

#define BACKTRACE_SIZE 16

/* 

Sources: 
- https://en.wikipedia.org/wiki/C_signal_handling
- https://hondu.co/blog/backtraces-in-c

 */

// Define a struct for the hash table entry
struct signal_map
{
    int signal;        // Signal number (key)
    char *description; // Description of the signal (value)
    UT_hash_handle hh; // Makes this structure hashable
};

// Declare the hash table (map)
struct signal_map *signal_descriptions = NULL;

// Add a signal-description pair to the map
void add_signal_description(int signal, char *description)
{
    struct signal_map *entry = malloc(sizeof(struct signal_map));
    if (entry == NULL) {
        perror("Failed to allocate memory for signal description");
        exit(EXIT_FAILURE);
    }
    entry->signal = signal;
    entry->description = description;
    HASH_ADD_INT(signal_descriptions, signal, entry); // Add key-value pair to the hash table
}

// Find the description for a signal
char *find_signal_description(int signal)
{
    struct signal_map *entry;
    HASH_FIND_INT(signal_descriptions, &signal, entry); // Find the element by signal number
    return entry ? entry->description : NULL;
}

// Free all signal descriptions from the hash table
void free_signal_descriptions()
{
    struct signal_map *current_entry, *tmp;
    HASH_ITER(hh, signal_descriptions, current_entry, tmp) {
        HASH_DEL(signal_descriptions, current_entry);  // Delete the entry from the hash table
        free(current_entry);  // Free the allocated memory
    }
}

void segfault_handler(int signal, siginfo_t *info, void *context) {
    // Access the memory address that caused the segmentation fault
    void *faulting_address = info->si_addr;

    // Print the faulting address
    if (faulting_address == NULL) {
        printf("Caught SIGSEGV: Segmentation fault at a NULL address. Trying to dereference a NULL pointer?\n");
    } else {
        printf("Caught SIGSEGV: Segmentation fault at address %p\n", faulting_address);
    }

    // Generate the backtrace - needs more investigation...
    void *buffer[BACKTRACE_SIZE];
    int nptrs = backtrace(buffer, BACKTRACE_SIZE);

    char **strs = backtrace_symbols(buffer, nptrs);
    for (int i = 0; i < nptrs; i++)
    {
        puts(strs[i]);
    }
    
    free(strs);

    // Save coverage data if available before exiting
    __coverage_save();

    // Exit the program after handling the fault
    exit(signal);
}

// Signal handler function
void crash_handler(int signal)
{
    char *description = find_signal_description(signal);
    if (description)
    {
        printf("Caught signal %d: %s\n", signal, description);
    }
    else
    {
        printf("Caught signal %d: Unknown signal\n", signal);
    }

    // For SIGABRT (assertion failures), save coverage data before exiting
    if (signal == SIGABRT) {
        printf("Assertion failure detected - saving coverage data\n");
        __coverage_save();
    }

    // Handle fatal signals by exiting the program
    if (signal == SIGSEGV || signal == SIGFPE || signal == SIGABRT || signal == SIGINT)
    {
        free_signal_descriptions(); // Free allocated memory before exiting
        exit(signal); // Exit with the signal code
    }
}

void initSignalHandler()
{
    add_signal_description(SIGINT, "Interrupt signal (Ctrl+C)");
    add_signal_description(SIGTERM, "Termination signal");
    add_signal_description(SIGSEGV, "Segmentation fault, something is trying to access memory it shouldn't!");
    add_signal_description(SIGFPE, "Floating point exception");
    add_signal_description(SIGABRT, "Abort signal");

    // Set up signal handlers using sigaction
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = segfault_handler;
    sigemptyset(&sa.sa_mask);

    // Set handler for SIGSEGV
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("Error setting SIGSEGV handler");
        exit(EXIT_FAILURE);
    }

    // Set up a common crash handler for other signals
    sa.sa_flags = 0;  // We don't need siginfo for other signals
    sa.sa_handler = crash_handler;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error setting SIGINT handler");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("Error setting SIGTERM handler");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGFPE, &sa, NULL) == -1) {
        perror("Error setting SIGFPE handler");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGABRT, &sa, NULL) == -1) {
        perror("Error setting SIGABRT handler");
        exit(EXIT_FAILURE);
    }
}

#endif // SIGNALS_H