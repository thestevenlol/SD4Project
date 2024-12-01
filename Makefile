CC = gcc
CFLAGS = -D_POSIX_C_SOURCE=199309L -g -rdynamic -fsanitize=undefined,address -O1
TARGET = output.o
SCANNER = scanner

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c

flex: flex.l
	flex flex.l
	$(CC) lex.yy.c -o $(SCANNER) -lfl

clean:
	rm -f $(TARGET) $(SCANNER) *.o lex.yy.c
