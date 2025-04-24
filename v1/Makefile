# Makefile for the Clang-based Fuzzer

# Use Clang as the compiler
CC = clang

# CFLAGS for the fuzzer executable itself
# -g for debugging, -Wall for warnings, -Iheaders to find header files
# Add -pthread if using pthreads later
FUZZER_CFLAGS = -g -Wall -Iheaders

# Linker flags for the fuzzer (none needed initially)
# Add -lrt if using shm_open (not needed for shmget)
# Add -pthread if using pthreads later
FUZZER_LDFLAGS =

# Source directory
SRC_DIR = src

# Header dependencies for the fuzzer source files
FUZZER_DEPS = headers/fuzz.h headers/io.h headers/testcase.h \
              headers/target.h headers/generational.h headers/logger.h \
              headers/corpus.h headers/coverage.h headers/uthash.h

# Source files for the fuzzer (excluding main.c which is in the root)
FUZZER_SRCS = $(SRC_DIR)/fuzz.c \
              $(SRC_DIR)/io.c \
              $(SRC_DIR)/testcase.c \
              $(SRC_DIR)/target.c \
              $(SRC_DIR)/generational.c \
              $(SRC_DIR)/logger.c \
              $(SRC_DIR)/corpus.c \
              $(SRC_DIR)/coverage.c
              # Note: coverage_runtime.c is NOT part of the fuzzer build
              # Note: range.c and lex.c removed

# Object files for the fuzzer
FUZZER_OBJS = $(FUZZER_SRCS:.c=.o)
MAIN_OBJ = main.o

# Default target: build the fuzzer executable 'main'
all: main

# Rule to compile fuzzer source files located in SRC_DIR
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(FUZZER_DEPS)
	@echo "Compiling fuzzer object $@"
	$(CC) $(FUZZER_CFLAGS) -c -o $@ $<

# Rule to compile main.c (located in the root directory)
$(MAIN_OBJ): main.c $(FUZZER_DEPS)
	@echo "Compiling fuzzer object $@"
	$(CC) $(FUZZER_CFLAGS) -c -o $@ $<

# Rule to link the fuzzer executable 'main'
main: $(MAIN_OBJ) $(FUZZER_OBJS)
	@echo "Linking fuzzer executable $@"
	$(CC) $(FUZZER_CFLAGS) -o $@ $^ $(FUZZER_LDFLAGS)
	@echo "Fuzzer executable 'main' built successfully."

# Rule to clean up generated files
clean:
	@echo "Cleaning up project..."
	rm -f $(MAIN_OBJ) $(FUZZER_OBJS)  # Remove fuzzer object files
	rm -f main                        # Remove fuzzer executable
	rm -f *_fuzz                      # Remove compiled target executables (pattern based)
	rm -f fuzzing_progress.csv        # Remove stats file
	rm -rf $(SRC_DIR)/coverage_runtime.o # Remove runtime object if accidentally created
	# Remove old flex/gcov artifacts just in case
	rm -f $(SRC_DIR)/lex.yy.c $(SRC_DIR)/scanner.o scanner lex.log
	rm -f *.gcno *.gcda *.gcov coverage/*.*
	# Optionally remove corpus and test-suites for a completely clean state
	rm -rf corpus/
	rm -rf test-suites/
	rm -f *~ core                     # Remove backup/core files
	@echo "Cleanup complete."

# Declare phony targets (targets that don't represent actual files)
.PHONY: all clean