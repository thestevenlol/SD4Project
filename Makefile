CC = gcc
CFLAGS = -g -Wall -fprofile-arcs -ftest-coverage -I.

SRC_DIR = src
DEPS = headers/fuzz.h headers/lex.h headers/io.h headers/testcase.h headers/target.h headers/range.h headers/generational.h headers/coverage.h headers/logger.h
OBJ = $(SRC_DIR)/main.o $(SRC_DIR)/fuzz.o $(SRC_DIR)/range.o $(SRC_DIR)/lex.o $(SRC_DIR)/io.o $(SRC_DIR)/testcase.o $(SRC_DIR)/target.o $(SRC_DIR)/generational.o $(SRC_DIR)/coverage.o $(SRC_DIR)/logger.o

all: main

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

flex: flex.l
	flex -o $(SRC_DIR)/lex.yy.c flex.l
	$(CC) -c $(SRC_DIR)/lex.yy.c -o $(SRC_DIR)/scanner.o
	$(CC) -o scanner $(SRC_DIR)/scanner.o

clean:
	rm -f $(SRC_DIR)/*.o *~ core $(SRC_DIR)/*.gcno $(SRC_DIR)/*.gcda $(SRC_DIR)/*.gcov temp_executable temp_executable* temp_coverage.txt main scanner $(SRC_DIR)/scanner.o $(SRC_DIR)/lex.yy.c

.PHONY: all clean flex
