CC = gcc
CFLAGS = -D_POSIX_C_SOURCE=199309L -g
TARGET = main

all: $(TARGET)

$(TARGET): main.o fuzz.o signals.o
	$(CC) $(CFLAGS) -o $(TARGET) main.o fuzz.o signals.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

fuzz.o: fuzz.c
	$(CC) $(CFLAGS) -c fuzz.c

signals.o: headers/signals.h
	$(CC) $(CFLAGS) -c headers/signals.h
clean:
	rm -f $(TARGET) *.o
