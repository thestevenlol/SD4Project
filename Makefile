CC = gcc
CFLAGS = -D_POSIX_C_SOURCE=199309L -g -rdynamic -fsanitize=undefined,address -O1
TARGET = output.o

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c

clean:
	rm -f $(TARGET) *.o