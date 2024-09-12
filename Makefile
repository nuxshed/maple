CC = gcc
CFLAGS = -Wall -Wextra -g
LIBS = -lncurses

TARGET = maple

all: $(TARGET)

$(TARGET): maple.c
	$(CC) $(CFLAGS) -o $(TARGET) maple.c $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
