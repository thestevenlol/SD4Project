CC = gcc
CFLAGS = -g -Wall -fprofile-arcs -ftest-coverage

SOURCES = main.c fuzz.c range.c lex.c io.c testcase.c target.c generational.c coverage.c logger.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = main

DEPS = headers/fuzz.h headers/lex.h headers/io.h headers/testcase.h headers/target.h headers/range.h headers/generational.h headers/coverage.h headers/logger.h

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

flex: flex.l
	flex -o lex.yy.c flex.l
	$(CC) -c lex.yy.c -o scanner.o
	$(CC) -o scanner scanner.o

clean:
	rm -f $(OBJECTS) $(TARGET) temp_executable temp_executable* temp_coverage.txt *.gcno *.gcda *.gcov scanner scanner.o lex.yy.c

.PHONY: all clean flex
