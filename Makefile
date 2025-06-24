CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99
TARGET = kilo

$(TARGET): kilo.c kilo.h
	$(CC) $(CFLAGS) kilo.c -o $(TARGET)

clean:
	rm -f $(TARGET)
