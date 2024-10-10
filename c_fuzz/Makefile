CC = gcc
CFLAGS = -D_POSIX_C_SOURCE=199309L
TARGET = main

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c

clean:
	rm -f $(TARGET)
