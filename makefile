CC = gcc
CFLAGS = -Wall
TARGET = disk

all: $(TARGET)

$(TARGET): disk.c
	$(CC) disk.c -o $(TARGET) $(CFLAGS)

clean:
	rm -f $(TARGET)
