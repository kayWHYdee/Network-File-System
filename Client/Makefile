CC = gcc
CFLAGS = -Wall -g
SOURCES = client.c copy.c create_delete.c list_paths.c access_file.c
TARGET = client

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET)

local:
	make clean && make && ./$(TARGET) 127.0.0.1

