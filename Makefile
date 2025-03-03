CC = gcc
CFLAGS = -g -Wall -fprofile-arcs -I.
LDFLAGS = -lgcov

SRC_DIR = src
DEPS = headers/fuzz.h headers/lex.h headers/io.h headers/testcase.h headers/target.h headers/range.h headers/generational.h headers/logger.h headers/corpus.h headers/coverage.h

# Separate main.o from other object files since it's in root directory
MAIN_OBJ = main.o
OBJ = $(SRC_DIR)/fuzz.o $(SRC_DIR)/range.o $(SRC_DIR)/lex.o $(SRC_DIR)/io.o $(SRC_DIR)/testcase.o $(SRC_DIR)/target.o $(SRC_DIR)/generational.o $(SRC_DIR)/logger.o $(SRC_DIR)/corpus.o $(SRC_DIR)/coverage.o

all: main

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main: $(MAIN_OBJ) $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

flex: flex.l
	flex -o $(SRC_DIR)/lex.yy.c flex.l
	$(CC) -c $(SRC_DIR)/lex.yy.c -o $(SRC_DIR)/scanner.o
	$(CC) -o scanner $(SRC_DIR)/scanner.o

clean:
	rm -f $(SRC_DIR)/*.o *~ core $(SRC_DIR)/*.gcno $(SRC_DIR)/*.gcda $(SRC_DIR)/*.gcov temp_executable temp_executable* temp_coverage.txt main scanner $(SRC_DIR)/scanner.o $(SRC_DIR)/lex.yy.c

.PHONY: all clean flex
