CC = gcc
CFLAGS = -Wall -g
# SOURCES = nm.c log.c trie.c cache.c handler.c
SOURCES = nm.c log.c trie.c cache.c 
TARGET = nm

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET)
run:
	./$(TARGET)

local:
	clear && make clean && make && ./$(TARGET) 127.0.0.1